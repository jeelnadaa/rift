// Pure logic shared by the background script, the popup and the tests (node browser/test.mjs).
// No browser APIs here.
(function (root) {
  "use strict";

  const HOST = "app.rift.native";

  const DEFAULTS = {
    enabled: true,        // this browser sends downloads to Rift at all
    takeOver: true,       // catch normal browser downloads
    types: "zip,rar,7z,tar,gz,xz,iso,img,exe,msi,dmg,pkg,deb,rpm,appimage,apk,mp4,mkv,avi,mov,webm,mp3,flac,wav,pdf,epub,torrent",
    minMB: 1,
    skipSites: [],        // hostnames where take-over is off
  };

  function parseTypes(s) {
    return String(s || "")
      .split(/[\s,;]+/)
      .map((t) => t.trim().toLowerCase().replace(/^\*?\./, ""))
      .filter(Boolean);
  }

  function hostOf(url) {
    try { return new URL(url).hostname.toLowerCase(); } catch (_) { return ""; }
  }

  function extensionOf(name) {
    const base = String(name || "").split(/[\\/]/).pop().split(/[?#]/)[0];
    const lower = base.toLowerCase();
    for (const multi of [".tar.gz", ".tar.xz", ".tar.bz2", ".tar.zst"]) if (lower.endsWith(multi)) return multi.slice(1);
    const i = lower.lastIndexOf(".");
    return i > 0 ? lower.slice(i + 1) : "";
  }

  function fileNameFromUrl(url) {
    try {
      const u = new URL(url);
      const last = u.pathname.split("/").filter(Boolean).pop() || "";
      return decodeURIComponent(last);
    } catch (_) { return ""; }
  }

  function isSupportedUrl(url) {
    return /^(https?|ftp):\/\//i.test(url) || /^magnet:\?/i.test(url);
  }

  function siteSkipped(url, skipSites) {
    const h = hostOf(url);
    if (!h) return false;
    return (skipSites || []).some((s) => {
      s = String(s).toLowerCase();
      return h === s || h.endsWith("." + s);
    });
  }

  // Only downloads that just started are taken over. Browsers also report downloads restored from history or
  // resumed after a restart; taking those caused a flood of take-overs when the browser opened.
  const FRESH_MS = 30000;

  // Decide whether a browser download should go to Rift instead.
  // item: { url, finalUrl, filename, mime, fileSize, totalBytes, incognito, byExtensionId, state, startTime }
  // cfg: merged settings (local + from Rift). now: ms (defaults to Date.now()). Returns { take, reason }.
  function shouldTakeOver(item, cfg, selfId, now) {
    const c = Object.assign({}, DEFAULTS, cfg || {});
    const url = item.finalUrl || item.url || "";
    const t = typeof now === "number" ? now : Date.now();
    if (!c.enabled) return { take: false, reason: "disabled" };
    if (!c.takeOver) return { take: false, reason: "take-over off" };
    if (item.byExtensionId && item.byExtensionId === selfId) return { take: false, reason: "ours" };
    if (item.incognito) return { take: false, reason: "private window" };
    if (item.state && item.state !== "in_progress") return { take: false, reason: "not new" };
    if (item.paused || item.canResume) return { take: false, reason: "resumed" };
    const started = Date.parse(item.startTime || "");
    if (!Number.isNaN(started) && t - started > FRESH_MS) return { take: false, reason: "old" };
    if (!isSupportedUrl(url)) return { take: false, reason: "unsupported url" };
    if (siteSkipped(url, c.skipSites) || siteSkipped(item.referrer || "", c.skipSites)) return { take: false, reason: "site skipped" };
    const name = item.filename || fileNameFromUrl(url);
    const ext = extensionOf(name);
    const types = parseTypes(c.types);
    const typeOk = types.length === 0 || types.includes(ext) || (ext.includes(".") && types.includes(ext.split(".").pop()))
      || (item.mime === "application/x-bittorrent" && types.includes("torrent"));
    if (!typeOk) return { take: false, reason: "type " + (ext || "unknown") };
    const size = Math.max(Number(item.fileSize) || 0, Number(item.totalBytes) || 0);
    const minBytes = (Number(c.minMB) || 0) * 1024 * 1024;
    // Unknown size (-1/0): take it; the user asked for this type.
    if (size > 0 && size < minBytes) return { take: false, reason: "small" };
    return { take: true, reason: "match" };
  }

  // Safety net around take-overs (all times in ms, pass `now` for tests):
  //  - a URL taken over (or handed back to the browser) isn't taken again for `repeatMs`, so a download the
  //    browser restarts after Rift failed can never loop;
  //  - at most `burstMax` take-overs per `burstMs`; the rest stay in the browser;
  //  - after Rift fails, nothing is taken over for `cooldownMs`.
  function createGuard(opts) {
    const o = Object.assign({ repeatMs: 10 * 60000, burstMs: 10000, burstMax: 6, cooldownMs: 2 * 60000 }, opts || {});
    const recent = new Map();
    let stamps = [];
    let downUntil = 0;
    function prune(now) {
      for (const [u, at] of recent) if (now - at > o.repeatMs) recent.delete(u);
      stamps = stamps.filter((at) => now - at < o.burstMs);
    }
    return {
      // urls: every URL the browser item has (url, finalUrl). Returns { ok, reason }.
      check(urls, now) {
        prune(now);
        if (now < downUntil) return { ok: false, reason: "rift unavailable" };
        for (const u of urls) if (u && recent.has(u)) return { ok: false, reason: "repeat" };
        if (stamps.length >= o.burstMax) return { ok: false, reason: "burst" };
        return { ok: true, reason: "" };
      },
      note(urls, now) {
        for (const u of urls) if (u) recent.set(u, now);
        stamps.push(now);
      },
      failed(now) { downUntil = now + o.cooldownMs; },
      succeeded() { downUntil = 0; },
      unavailable(now) { return now < downUntil; },
    };
  }

  function cookieHeader(cookies) {
    return (cookies || [])
      .filter((c) => c && c.name)
      .map((c) => c.name + "=" + c.value)
      .join("; ");
  }

  function downloadMessage(url, opts) {
    const o = opts || {};
    const msg = { type: "download", url: String(url) };
    if (o.referer) msg.referer = String(o.referer);
    if (o.cookies) msg.cookies = String(o.cookies);
    if (o.userAgent) msg.userAgent = String(o.userAgent);
    if (o.filename) msg.filename = String(o.filename).split(/[\\/]/).pop();
    return msg;
  }

  // Unique, supported links from a list (used for "Download all links").
  function collectLinks(urls, types) {
    const t = parseTypes(types);
    const seen = new Set();
    const out = [];
    for (const u of urls || []) {
      if (!isSupportedUrl(u) || seen.has(u)) continue;
      if (!/^magnet:/i.test(u) && t.length && !t.includes(extensionOf(fileNameFromUrl(u)))) continue;
      seen.add(u);
      out.push(u);
    }
    return out;
  }

  const api = { HOST, DEFAULTS, parseTypes, hostOf, extensionOf, fileNameFromUrl, isSupportedUrl, siteSkipped,
                shouldTakeOver, createGuard, cookieHeader, downloadMessage, collectLinks, FRESH_MS };
  if (typeof module === "object" && module.exports) module.exports = api;
  root.RiftLogic = api;
})(typeof globalThis !== "undefined" ? globalThis : this);

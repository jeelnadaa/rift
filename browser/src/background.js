// Rift browser extension background (Chromium service worker / Firefox event page).
// Talks to Rift only through Native Messaging (host "app.rift.native"); no network requests of its own.
/* global RiftLogic, importScripts */
if (typeof importScripts === "function" && typeof RiftLogic === "undefined") importScripts("logic.js");

const api = typeof browser !== "undefined" ? browser : chrome;
const L = RiftLogic;
const guard = L.createGuard();       // no loops, no bursts, back off while Rift is unreachable
let remoteCfg = null;               // last config from Rift
let remoteAt = 0;
let remoteFailAt = 0;               // last failed config request (don't ask again for a minute)
let takeovers = Promise.resolve();  // take-overs run one at a time

// ---- Native messaging ------------------------------------------------------------------------
function native(msg) {
  return new Promise((resolve) => {
    const fail = (e) => resolve({ ok: false, error: String((e && e.message) || e), missing: true });
    try {
      if (typeof browser !== "undefined") {
        // Firefox: promise form only.
        browser.runtime.sendNativeMessage(L.HOST, msg).then((r) => resolve(r || { ok: false, error: "no reply" }), fail);
        return;
      }
      chrome.runtime.sendNativeMessage(L.HOST, msg, (reply) => {
        const err = chrome.runtime.lastError;
        if (err) fail(err); else resolve(reply || { ok: false, error: "no reply" });
      });
    } catch (e) {
      resolve({ ok: false, error: String(e && e.message || e), missing: true });
    }
  });
}

async function localCfg() {
  const s = await api.storage.local.get(Object.keys(L.DEFAULTS));
  return Object.assign({}, L.DEFAULTS, s);
}

// Rift's own settings win for types / size; the extension's toggles can only narrow them.
async function effectiveCfg() {
  const local = await localCfg();
  const now = Date.now();
  if ((!remoteCfg || now - remoteAt > 60000) && now - remoteFailAt > 60000) {
    // "config" never starts Rift (the native host only launches it for a real download).
    const r = await native({ type: "config" });
    if (r && r.ok) { remoteCfg = r; remoteAt = Date.now(); } else remoteFailAt = Date.now();
  }
  const c = Object.assign({}, local);
  if (remoteCfg) {
    c.enabled = local.enabled && remoteCfg.enabled !== false;
    c.takeOver = local.takeOver && remoteCfg.takeOver !== false;
    if (remoteCfg.types) c.types = remoteCfg.types;
    if (typeof remoteCfg.minMB === "number") c.minMB = remoteCfg.minMB;
  }
  return c;
}

async function cookiesFor(url) {
  try {
    if (!/^https?:/i.test(url)) return "";
    const list = await api.cookies.getAll({ url });
    return L.cookieHeader(list);
  } catch (_) { return ""; }
}

async function sendToRift(url, extra) {
  const e = extra || {};
  const msg = L.downloadMessage(url, {
    referer: e.referer || "",
    cookies: await cookiesFor(url),
    userAgent: navigator.userAgent,
    filename: e.filename || "",
  });
  const r = await native(msg);
  await setBadge(r && r.ok ? "ok" : "err");
  return r;
}

async function setBadge(kind) {
  try {
    const a = api.action || api.browserAction;
    if (kind === "ok") {
      await a.setBadgeBackgroundColor({ color: "#1E46D8" });
      await a.setBadgeText({ text: "↓" });
      setTimeout(() => a.setBadgeText({ text: "" }), 2500);
    } else if (kind === "err") {
      await a.setBadgeBackgroundColor({ color: "#B32638" });
      await a.setBadgeText({ text: "!" });
      setTimeout(() => a.setBadgeText({ text: "" }), 5000);
    }
  } catch (_) { /* badge is cosmetic */ }
}

// ---- Context menus ---------------------------------------------------------------------------
function setupMenus() {
  const menus = api.contextMenus || api.menus;
  if (!menus) return;
  menus.removeAll(() => {
    menus.create({ id: "rift-link", title: "Download with Rift", contexts: ["link"] });
    menus.create({ id: "rift-media", title: "Download with Rift", contexts: ["image", "video", "audio"] });
    menus.create({ id: "rift-all", title: "Download all links with Rift", contexts: ["page"] });
    menus.create({ id: "rift-selection", title: "Send selected links to Rift", contexts: ["selection"] });
  });
}

async function linksInTab(tabId, selectionOnly) {
  try {
    const [res] = await api.scripting.executeScript({
      target: { tabId },
      func: (onlySel) => {
        let nodes = Array.from(document.querySelectorAll("a[href]"));
        if (onlySel) {
          const sel = window.getSelection();
          nodes = nodes.filter((a) => sel && sel.containsNode(a, true));
        }
        return nodes.map((a) => a.href);
      },
      args: [!!selectionOnly],
    });
    return (res && res.result) || [];
  } catch (_) { return []; }
}

async function onMenu(info, tab) {
  const referer = (tab && tab.url) || info.pageUrl || "";
  if (info.menuItemId === "rift-link" && info.linkUrl) return sendToRift(info.linkUrl, { referer });
  if (info.menuItemId === "rift-media" && info.srcUrl) return sendToRift(info.srcUrl, { referer });
  if ((info.menuItemId === "rift-all" || info.menuItemId === "rift-selection") && tab) {
    const cfg = await effectiveCfg();
    const all = await linksInTab(tab.id, info.menuItemId === "rift-selection");
    const links = L.collectLinks(all, info.menuItemId === "rift-selection" ? "" : cfg.types);
    for (const u of links.slice(0, 500)) {
      const r = await sendToRift(u, { referer });
      if (!r || !r.ok) break; // Rift is unavailable: don't try the other 499
    }
  }
}

// ---- Take over browser downloads -------------------------------------------------------------
async function takeOver(item) {
  const url = item.finalUrl || item.url;
  const urls = [item.url, item.finalUrl];
  const pre = guard.check(urls, Date.now());
  if (!pre.ok) return;                          // repeat / burst / Rift unavailable: the browser keeps it
  const cfg = await effectiveCfg();
  const d = L.shouldTakeOver(item, cfg, api.runtime.id, Date.now());
  if (!d.take) return;
  const g = guard.check(urls, Date.now());      // re-check: state may have changed while asking for config
  if (!g.ok) return;
  guard.note(urls, Date.now());
  try { await api.downloads.cancel(item.id); } catch (_) { return; }  // already finished: leave it
  try { await api.downloads.erase({ id: item.id }); } catch (_) { /* ignore */ }
  const r = await sendToRift(url, { referer: item.referrer || "", filename: item.filename || "" });
  if (r && r.ok) { guard.succeeded(); return; }
  // Rift is unavailable: give this one download back to the browser (the guard has its URL, so the restarted
  // download isn't taken again) and leave every other download alone for a while.
  guard.failed(Date.now());
  try { await api.downloads.download({ url }); } catch (_) { /* nothing else we can do */ }
}

function onCreated(item) {
  // One at a time, so a burst of downloads can't start a burst of native hosts.
  takeovers = takeovers.then(() => takeOver(item)).catch(() => {});
}

// ---- Messages from the popup / options ------------------------------------------------------
api.runtime.onMessage.addListener((msg, _sender, reply) => {
  (async () => {
    if (msg && msg.type === "status") {
      const r = await native({ type: "ping" });
      reply({ connected: !!(r && r.ok), installed: !(r && r.missing), version: r && r.version, error: r && r.error, cfg: await localCfg() });
    } else if (msg && msg.type === "send" && msg.url) {
      reply(await sendToRift(msg.url, { referer: msg.referer || "" }));
    } else if (msg && msg.type === "refresh") {
      remoteCfg = null;
      reply({ ok: true });
    } else reply({ ok: false });
  })();
  return true; // async reply
});

(api.contextMenus || api.menus).onClicked.addListener(onMenu);
api.downloads.onCreated.addListener(onCreated);
api.runtime.onInstalled.addListener(setupMenus);
if (api.runtime.onStartup) api.runtime.onStartup.addListener(setupMenus);

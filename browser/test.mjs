// Tests for the extension: pure logic + both built manifests.   node browser/test.mjs
import assert from "node:assert/strict";
import { createRequire } from "node:module";
import { existsSync, readFileSync, readdirSync } from "node:fs";
import { join } from "node:path";
import { build, CHROMIUM_ID, FIREFOX_ID } from "./build.mjs";
import { createHash } from "node:crypto";

const require = createRequire(import.meta.url);
const L = require("./src/logic.js");
let n = 0;
const t = (name, fn) => { fn(); n++; };

t("parseTypes", () => {
  assert.deepEqual(L.parseTypes(" .ZIP, *.iso;mp4  rar "), ["zip", "iso", "mp4", "rar"]);
  assert.deepEqual(L.parseTypes(""), []);
});
t("extensionOf", () => {
  assert.equal(L.extensionOf("a/b/file.TAR.GZ"), "tar.gz");
  assert.equal(L.extensionOf("x.iso?token=1"), "iso");
  assert.equal(L.extensionOf("README"), "");
  assert.equal(L.extensionOf(".hidden"), "");
});
t("fileNameFromUrl", () => {
  assert.equal(L.fileNameFromUrl("https://e.com/a/My%20File.zip?x=1"), "My File.zip");
  assert.equal(L.fileNameFromUrl("not a url"), "");
});
t("isSupportedUrl", () => {
  assert.ok(L.isSupportedUrl("https://e.com/a"));
  assert.ok(L.isSupportedUrl("ftp://e.com/a"));
  assert.ok(L.isSupportedUrl("magnet:?xt=urn:btih:abc"));
  assert.ok(!L.isSupportedUrl("blob:https://e.com/1"));
  assert.ok(!L.isSupportedUrl("data:text/plain,hi"));
  assert.ok(!L.isSupportedUrl("javascript:alert(1)"));
});
t("siteSkipped", () => {
  assert.ok(L.siteSkipped("https://dl.example.com/x", ["example.com"]));
  assert.ok(!L.siteSkipped("https://notexample.com/x", ["example.com"]));
  assert.ok(!L.siteSkipped("", ["example.com"]));
});
t("shouldTakeOver skips downloads that aren't new", () => {
  const now = Date.parse("2026-11-07T20:00:00Z");
  const fresh = { url: "https://e.com/a.iso", fileSize: 3e9, state: "in_progress", startTime: "2026-11-07T19:59:55Z" };
  assert.equal(L.shouldTakeOver(fresh, {}, "", now).take, true);
  assert.equal(L.shouldTakeOver({ ...fresh, startTime: "2026-11-07T19:50:00Z" }, {}, "", now).reason, "old");
  assert.equal(L.shouldTakeOver({ ...fresh, state: "interrupted" }, {}, "", now).reason, "not new");
  assert.equal(L.shouldTakeOver({ ...fresh, state: "complete" }, {}, "", now).reason, "not new");
  assert.equal(L.shouldTakeOver({ ...fresh, paused: true }, {}, "", now).reason, "resumed");
  assert.equal(L.shouldTakeOver({ ...fresh, startTime: undefined }, {}, "", now).take, true);
});
t("guard: no repeats, no bursts, backs off after a failure", () => {
  const g = L.createGuard({ repeatMs: 1000, burstMs: 1000, burstMax: 3, cooldownMs: 5000 });
  assert.equal(g.check(["https://e.com/a"], 0).ok, true);
  g.note(["https://e.com/a", undefined], 0);
  assert.equal(g.check(["https://e.com/a"], 10).reason, "repeat");
  assert.equal(g.check([undefined, "https://e.com/a"], 10).reason, "repeat");
  assert.equal(g.check(["https://e.com/a"], 2000).ok, true);
  g.note(["https://e.com/b"], 2000); g.note(["https://e.com/c"], 2001); g.note(["https://e.com/d"], 2002);
  assert.equal(g.check(["https://e.com/e"], 2003).reason, "burst");
  assert.equal(g.check(["https://e.com/e"], 3500).ok, true);
  g.failed(4000);
  assert.equal(g.check(["https://e.com/z"], 4001).reason, "rift unavailable");
  assert.equal(g.unavailable(8999), true);
  assert.equal(g.check(["https://e.com/z"], 9001).ok, true);
  g.failed(10000); g.succeeded();
  assert.equal(g.check(["https://e.com/y"], 10001).ok, true);
});
t("shouldTakeOver", () => {
  const big = { url: "https://e.com/ubuntu.iso", fileSize: 3e9 };
  assert.equal(L.shouldTakeOver(big, {}).take, true);
  assert.equal(L.shouldTakeOver(big, { takeOver: false }).take, false);
  assert.equal(L.shouldTakeOver(big, { enabled: false }).take, false);
  assert.equal(L.shouldTakeOver({ ...big, incognito: true }, {}).take, false);
  assert.equal(L.shouldTakeOver({ ...big, byExtensionId: "me" }, {}, "me").take, false);
  assert.equal(L.shouldTakeOver({ url: "https://e.com/page.html", fileSize: 3e9 }, {}).take, false);
  assert.equal(L.shouldTakeOver({ url: "https://e.com/a.zip", fileSize: 1000 }, { minMB: 1 }).take, false);
  assert.equal(L.shouldTakeOver({ url: "https://e.com/a.zip", fileSize: -1 }, { minMB: 1 }).take, true);
  assert.equal(L.shouldTakeOver({ url: "blob:https://e.com/1", filename: "a.zip" }, {}).take, false);
  assert.equal(L.shouldTakeOver({ url: "https://e.com/get?id=1", mime: "application/x-bittorrent" }, {}).take, true);
  assert.equal(L.shouldTakeOver({ url: "https://e.com/x.tar.gz", fileSize: 5e6 }, { types: "gz" }).take, true);
  assert.equal(L.shouldTakeOver(big, { skipSites: ["e.com"] }).take, false);
  assert.equal(L.shouldTakeOver({ url: "https://e.com/a.bin", finalUrl: "https://cdn.e.com/a.iso", fileSize: 3e9 }, {}).take, true);
});
t("cookieHeader", () => {
  assert.equal(L.cookieHeader([{ name: "a", value: "1" }, { name: "b", value: "2" }, null, { value: "x" }]), "a=1; b=2");
});
t("downloadMessage", () => {
  const m = L.downloadMessage("https://e.com/a.zip", { referer: "https://e.com", filename: "C:\\x\\a.zip", cookies: "a=1" });
  assert.deepEqual(m, { type: "download", url: "https://e.com/a.zip", referer: "https://e.com", cookies: "a=1", filename: "a.zip" });
  assert.deepEqual(L.downloadMessage("magnet:?xt=1"), { type: "download", url: "magnet:?xt=1" });
});
t("collectLinks", () => {
  const got = L.collectLinks(["https://e.com/a.zip", "https://e.com/a.zip", "https://e.com/p.html", "magnet:?xt=1", "javascript:x"], "zip");
  assert.deepEqual(got, ["https://e.com/a.zip", "magnet:?xt=1"]);
});

t("built manifests", () => {
  const out = build("1.2.3");
  const c = JSON.parse(readFileSync(join(out, "chromium", "manifest.json"), "utf8"));
  const f = JSON.parse(readFileSync(join(out, "firefox", "manifest.json"), "utf8"));
  for (const m of [c, f]) {
    assert.equal(m.manifest_version, 3);
    assert.equal(m.version, "1.2.3");
    assert.ok(m.permissions.includes("nativeMessaging"));
    assert.ok(m.permissions.includes("downloads"));
  }
  assert.equal(c.background.service_worker, "background.js");
  assert.deepEqual(f.background.scripts, ["logic.js", "background.js"]);
  assert.equal(f.browser_specific_settings.gecko.id, FIREFOX_ID);
  // The key must produce the ID that Rift's native-messaging manifest allows.
  const der = Buffer.from(c.key, "base64");
  const id = createHash("sha256").update(der).digest("hex").slice(0, 32).replace(/./g, (h) => "abcdefghijklmnop"[parseInt(h, 16)]);
  assert.equal(id, CHROMIUM_ID);
  const platform = readFileSync(new URL("../src/app/platform.hpp", import.meta.url), "utf8");
  assert.ok(platform.includes(CHROMIUM_ID), "platform.hpp allows the Chromium ID");
  assert.ok(platform.includes(FIREFOX_ID), "platform.hpp allows the Firefox ID");
  for (const target of ["chromium", "firefox"]) {
    const files = readdirSync(join(out, target));
    for (const need of ["background.js", "logic.js", "popup.html", "popup.js", "options.html", "options.js", "style.css", "icons"])
      assert.ok(files.includes(need), target + " has " + need);
    assert.ok(!files.includes("manifest.base.json"));
    // Every file referenced from HTML exists
    for (const html of ["popup.html", "options.html"]) {
      const s = readFileSync(join(out, target, html), "utf8");
      for (const [, ref] of s.matchAll(/(?:src|href)="([^"#:]+)"/g)) assert.ok(existsSync(join(out, target, ref)), html + " → " + ref);
    }
  }
});

console.log("browser tests: " + n + " passed");

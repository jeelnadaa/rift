/* global RiftLogic */
const api = typeof browser !== "undefined" ? browser : chrome;
const $ = (id) => document.getElementById(id);
let host = "";

function send(msg) { return new Promise((res) => api.runtime.sendMessage(msg, res)); }
function say(text, kind) { const m = $("msg"); m.textContent = text; m.className = kind || ""; }

async function load() {
  const [tab] = await api.tabs.query({ active: true, currentWindow: true });
  host = tab && tab.url ? RiftLogic.hostOf(tab.url) : "";
  $("site").textContent = host || "Not a web page";

  const st = await send({ type: "status" });
  const cfg = (st && st.cfg) || RiftLogic.DEFAULTS;
  const s = $("status");
  if (st && st.connected) { s.className = "status ok"; s.lastElementChild.textContent = "Connected"; }
  else if (st && st.installed) { s.className = "status off"; s.lastElementChild.textContent = "Not running"; }
  else { s.className = "status missing"; s.lastElementChild.textContent = "Not found"; $("missing").hidden = false; }
  $("ver").textContent = st && st.version ? "Rift " + st.version : "";

  $("takeOver").checked = cfg.takeOver !== false;
  $("siteOn").checked = !RiftLogic.siteSkipped("https://" + host + "/", cfg.skipSites);
  $("siteOn").disabled = !host;
}

$("takeOver").addEventListener("change", async (e) => {
  await api.storage.local.set({ takeOver: e.target.checked });
  await send({ type: "refresh" });
});
$("siteOn").addEventListener("change", async (e) => {
  const { skipSites = [] } = await api.storage.local.get("skipSites");
  const next = skipSites.filter((s) => s !== host);
  if (!e.target.checked && host) next.push(host);
  await api.storage.local.set({ skipSites: next });
});
$("send").addEventListener("submit", async (e) => {
  e.preventDefault();
  const url = $("url").value.trim();
  if (!RiftLogic.isSupportedUrl(url)) return say("That isn't a link or magnet Rift understands.", "err");
  say("Sending…");
  const r = await send({ type: "send", url });
  if (r && r.ok) { say("Sent to Rift.", "ok"); $("url").value = ""; }
  else say((r && r.error) || "Rift didn't answer.", "err");
});
$("options").addEventListener("click", () => api.runtime.openOptionsPage());
load();

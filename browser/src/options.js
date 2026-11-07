/* global RiftLogic */
const api = typeof browser !== "undefined" ? browser : chrome;
const $ = (id) => document.getElementById(id);

async function load() {
  const c = Object.assign({}, RiftLogic.DEFAULTS, await api.storage.local.get(Object.keys(RiftLogic.DEFAULTS)));
  $("enabled").checked = c.enabled !== false;
  $("takeOver").checked = c.takeOver !== false;
  $("types").value = RiftLogic.parseTypes(c.types).join(", ");
  $("minMB").value = c.minMB;
  $("skipSites").value = (c.skipSites || []).join("\n");
}
async function save() {
  await api.storage.local.set({
    enabled: $("enabled").checked,
    takeOver: $("takeOver").checked,
    types: RiftLogic.parseTypes($("types").value).join(","),
    minMB: Math.max(0, Number($("minMB").value) || 0),
    skipSites: $("skipSites").value.split(/\s+/).map((s) => s.trim().toLowerCase()).filter(Boolean),
  });
  api.runtime.sendMessage({ type: "refresh" });
  const s = $("saved"); s.classList.add("on"); setTimeout(() => s.classList.remove("on"), 1600);
}
$("save").addEventListener("click", save);
$("reset").addEventListener("click", async () => { await api.storage.local.clear(); await load(); await save(); });
load();

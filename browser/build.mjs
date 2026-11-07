// Build the Rift extension for Chromium (Chrome, Edge, Brave, Vivaldi) and Firefox from browser/src.
//   node browser/build.mjs [version]       → dist/browser/chromium, dist/browser/firefox
// The version defaults to $RIFT_VERSION or 0.0.0. Fonts from ui/fonts are bundled when present.
import { cpSync, existsSync, mkdirSync, readFileSync, readdirSync, rmSync, writeFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const here = dirname(fileURLToPath(import.meta.url));
const root = join(here, "..");
const src = join(here, "src");
const out = join(root, "dist", "browser");
const version = (process.argv[2] || process.env.RIFT_VERSION || "0.0.0").replace(/^v/, "").replace(/[^0-9.].*$/, "") || "0.0.0";

// Chromium's stable extension ID (llhjnffjocckdmklgdcpnllflgcacppo) comes from this public key.
// Rift's native-messaging manifest allows exactly that ID, and Firefox's browser@rift.app.
export const CHROMIUM_ID = "llhjnffjocckdmklgdcpnllflgcacppo";
export const FIREFOX_ID = "browser@rift.app";

const FONT_FILES = {
  "Bricolage Grotesque": /^BricolageGrotesque.*\.ttf$/i,
  "JetBrains Mono": /^JetBrainsMono.*\.ttf$/i,
};

function fontCss(fontDir) {
  if (!existsSync(fontDir)) return { css: "", files: [] };
  const files = [];
  let css = "";
  for (const f of readdirSync(fontDir)) {
    for (const [family, re] of Object.entries(FONT_FILES)) {
      if (!re.test(f) || /italic/i.test(f)) continue;
      files.push(f);
      css += `@font-face { font-family: "${family}"; src: url("fonts/${f}") format("truetype"); font-weight: 100 900; font-display: swap; }\n`;
    }
  }
  return { css, files };
}

export function manifests(ver) {
  const base = JSON.parse(readFileSync(join(src, "manifest.base.json"), "utf8"));
  base.version = ver;
  const chromium = structuredClone(base);
  chromium.key = readFileSync(join(here, "chromium-key.txt"), "utf8").trim();
  chromium.background = { service_worker: "background.js" };
  chromium.minimum_chrome_version = "110";
  const firefox = structuredClone(base);
  firefox.background = { scripts: ["logic.js", "background.js"] };
  firefox.browser_specific_settings = { gecko: { id: FIREFOX_ID, strict_min_version: "115.0" } };
  firefox.permissions = firefox.permissions.filter((p) => p !== "contextMenus").concat(["menus"]);
  return { chromium, firefox };
}

export function build(ver = version) {
  const m = manifests(ver);
  const fonts = fontCss(join(root, "ui", "fonts"));
  for (const target of ["chromium", "firefox"]) {
    const dir = join(out, target);
    rmSync(dir, { recursive: true, force: true });
    mkdirSync(dir, { recursive: true });
    cpSync(src, dir, { recursive: true, filter: (p) => !p.endsWith("manifest.base.json") });
    writeFileSync(join(dir, "manifest.json"), JSON.stringify(m[target], null, 2) + "\n");
    if (fonts.files.length) {
      mkdirSync(join(dir, "fonts"), { recursive: true });
      for (const f of fonts.files) cpSync(join(root, "ui", "fonts", f), join(dir, "fonts", f));
      const css = readFileSync(join(dir, "style.css"), "utf8");
      writeFileSync(join(dir, "style.css"), fonts.css + css);
    }
  }
  return out;
}

if (process.argv[1] && fileURLToPath(import.meta.url) === process.argv[1]) {
  console.log("Built Rift extension " + version + " → " + build());
}

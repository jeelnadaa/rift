/* Rift app design — icon set + prototype shell behaviors.
   Icons: 24x24 grid, 1.75 stroke, round caps/joins. Export each as SVG for the app (src/ui/assets/icons). */
(() => {
const I = {
  logo: '<path transform="translate(-1.1 -4.6) scale(.375)" d="M39.24 6.66A17 28 0 1 0 24.76 57.34Z" style="fill:currentColor;stroke:none"/><path transform="translate(1.1 4.6) scale(.375)" d="M39.24 6.66A17 28 0 1 1 24.76 57.34Z" style="fill:currentColor;stroke:none;opacity:.6"/>',
  plus: '<path d="M12 5v14M5 12h14"/>',
  link: '<path d="M10 14a4 4 0 0 0 5.66 0l3-3a4 4 0 0 0-5.66-5.66l-1 1"/><path d="M14 10a4 4 0 0 0-5.66 0l-3 3a4 4 0 0 0 5.66 5.66l1-1"/>',
  magnet: '<path d="M6 3h4v8a2 2 0 0 0 4 0V3h4v8a6 6 0 0 1-12 0z"/><path d="M6 7h4M14 7h4"/>',
  file: '<path d="M14 3H7a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h10a2 2 0 0 0 2-2V8z"/><path d="M14 3v5h5"/>',
  fileTorrent: '<path d="M14 3H7a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h10a2 2 0 0 0 2-2V8z"/><path d="M14 3v5h5"/><path d="M13 11l-3 4h4l-3 4"/>',
  folder: '<path d="M3 7a2 2 0 0 1 2-2h4l2 2h8a2 2 0 0 1 2 2v8a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z"/>',
  folderOpen: '<path d="M3 17V7a2 2 0 0 1 2-2h4l2 2h7a2 2 0 0 1 2 2v1"/><path d="M3 17l2.5-6.5A2 2 0 0 1 7.4 9H21l-3 8a2 2 0 0 1-1.9 1.3H5A2 2 0 0 1 3 17z"/>',
  play: '<path d="M7 4v16l13-8z"/>',
  pause: '<path d="M8 4v16M16 4v16"/>',
  stop: '<rect x="6" y="6" width="12" height="12" rx="1.5"/>',
  trash: '<path d="M4 7h16M10 11v6M14 11v6M6 7l1 12a2 2 0 0 0 2 2h6a2 2 0 0 0 2-2l1-12M9 7V4h6v3"/>',
  search: '<circle cx="11" cy="11" r="7"/><path d="m20 20-3.5-3.5"/>',
  settings: '<circle cx="12" cy="12" r="3"/><path d="M19.4 15a1.7 1.7 0 0 0 .3 1.8l.1.1a2 2 0 1 1-2.8 2.8l-.1-.1a1.7 1.7 0 0 0-1.8-.3 1.7 1.7 0 0 0-1 1.5V21a2 2 0 1 1-4 0v-.1a1.7 1.7 0 0 0-1.1-1.5 1.7 1.7 0 0 0-1.8.3l-.1.1a2 2 0 1 1-2.8-2.8l.1-.1a1.7 1.7 0 0 0 .3-1.8 1.7 1.7 0 0 0-1.5-1H3a2 2 0 1 1 0-4h.1a1.7 1.7 0 0 0 1.5-1.1 1.7 1.7 0 0 0-.3-1.8l-.1-.1a2 2 0 1 1 2.8-2.8l.1.1a1.7 1.7 0 0 0 1.8.3H9a1.7 1.7 0 0 0 1-1.5V3a2 2 0 1 1 4 0v.1a1.7 1.7 0 0 0 1 1.5 1.7 1.7 0 0 0 1.8-.3l.1-.1a2 2 0 1 1 2.8 2.8l-.1.1a1.7 1.7 0 0 0-.3 1.8V9a1.7 1.7 0 0 0 1.5 1H21a2 2 0 1 1 0 4h-.1a1.7 1.7 0 0 0-1.5 1z"/>',
  filter: '<path d="M3 5h18l-7 8v6l-4 2v-8z"/>',
  chevD: '<path d="m6 9 6 6 6-6"/>', chevR: '<path d="m9 6 6 6-6 6"/>', chevU: '<path d="m6 15 6-6 6 6"/>', chevL: '<path d="m15 6-6 6 6 6"/>',
  arrowD: '<path d="M12 4v16M6 14l6 6 6-6"/>', arrowU: '<path d="M12 20V4M6 10l6-6 6 6"/>',
  arrowUp2: '<path d="M12 19V5M5 12l7-7 7 7"/>', arrowTop: '<path d="M5 4h14M12 20V9M7 13l5-5 5 5"/>', arrowBottom: '<path d="M5 20h14M12 4v11M7 11l5 5 5-5"/>',
  x: '<path d="M6 6l12 12M18 6 6 18"/>', check: '<path d="m5 12 5 5 9-10"/>',
  more: '<circle cx="5" cy="12" r="1.2"/><circle cx="12" cy="12" r="1.2"/><circle cx="19" cy="12" r="1.2"/>',
  grip: '<circle cx="9" cy="6" r="1"/><circle cx="15" cy="6" r="1"/><circle cx="9" cy="12" r="1"/><circle cx="15" cy="12" r="1"/><circle cx="9" cy="18" r="1"/><circle cx="15" cy="18" r="1"/>',
  sun: '<circle cx="12" cy="12" r="4.5"/><path d="M12 2v2.5M12 19.5V22M4.2 4.2 6 6M18 18l1.8 1.8M2 12h2.5M19.5 12H22M4.2 19.8 6 18M18 6l1.8-1.8"/>',
  moon: '<path d="M20.5 13.2A8.5 8.5 0 1 1 10.8 3.5a6.6 6.6 0 0 0 9.7 9.7z"/>',
  turtle: '<path d="M4 15h13a3 3 0 0 0 3-3v0a1 1 0 0 1 1-1h1"/><path d="M5 15a7 7 0 0 1 14 0"/><path d="M7 15v3M15 15v3"/><path d="M9 9l2 3 2-3"/>',
  bolt: '<path d="M13 2 4 14h7l-1 8 9-12h-7z"/>',
  shield: '<path d="M12 3 4 6v6c0 5 3.5 8 8 9 4.5-1 8-4 8-9V6z"/>',
  shieldOff: '<path d="M12 3 4 6v6c0 5 3.5 8 8 9 4.5-1 8-4 8-9V6z"/><path d="m4 4 16 16"/>',
  globe: '<circle cx="12" cy="12" r="9"/><path d="M3 12h18M12 3a14 14 0 0 1 0 18M12 3a14 14 0 0 0 0 18"/>',
  wifi: '<path d="M2 9a15 15 0 0 1 20 0M5 12.5a10 10 0 0 1 14 0M8.5 16a5 5 0 0 1 7 0"/><circle cx="12" cy="19" r=".6"/>',
  rss: '<path d="M5 5a14 14 0 0 1 14 14M5 11a8 8 0 0 1 8 8"/><circle cx="6" cy="18" r="1.2"/>',
  chart: '<path d="M4 20V10M10 20V4M16 20v-7M22 20H2"/>',
  graph: '<path d="M3 17l5-6 4 4 4-7 5 6"/><path d="M3 21h18"/>',
  tag: '<path d="M3 12V4a1 1 0 0 1 1-1h8l9 9-9 9z"/><circle cx="8" cy="8" r="1.4"/>',
  layers: '<path d="m12 3 9 5-9 5-9-5z"/><path d="m3 13 9 5 9-5"/>',
  clock: '<circle cx="12" cy="12" r="9"/><path d="M12 7v5l3 2"/>',
  calendar: '<rect x="3" y="5" width="18" height="16" rx="2"/><path d="M3 10h18M8 3v4M16 3v4"/>',
  cpu: '<rect x="6" y="6" width="12" height="12" rx="2"/><path d="M9 2v4M15 2v4M9 18v4M15 18v4M2 9h4M2 15h4M18 9h4M18 15h4"/>',
  disk: '<rect x="3" y="13" width="18" height="7" rx="2"/><path d="M5 13 7.5 5h9L19 13"/><circle cx="17" cy="16.5" r=".7"/>',
  download: '<path d="M12 3v12M7 10l5 5 5-5M4 21h16"/>',
  upload: '<path d="M12 21V9M7 14l5-5 5 5M4 3h16"/>',
  eye: '<path d="M2 12s3.5-7 10-7 10 7 10 7-3.5 7-10 7S2 12 2 12z"/><circle cx="12" cy="12" r="3"/>',
  copy: '<rect x="8" y="8" width="13" height="13" rx="2"/><path d="M16 8V5a2 2 0 0 0-2-2H5a2 2 0 0 0-2 2v9a2 2 0 0 0 2 2h3"/>',
  refresh: '<path d="M20 11a8 8 0 0 0-14.9-3M4 13a8 8 0 0 0 14.9 3"/><path d="M4 4v4h4M20 20v-4h-4"/>',
  recheck: '<path d="M20 11a8 8 0 1 0-2.3 5.7"/><path d="M20 4v7h-7"/><path d="m8.5 12 2.5 2.5 4.5-5"/>',
  info: '<circle cx="12" cy="12" r="9"/><path d="M12 11v5M12 8h.01"/>',
  alert: '<path d="M12 3 2 20h20z"/><path d="M12 10v4M12 17h.01"/>',
  alertCircle: '<circle cx="12" cy="12" r="9"/><path d="M12 7v6M12 16h.01"/>',
  list: '<path d="M8 6h13M8 12h13M8 18h13M3 6h.01M3 12h.01M3 18h.01"/>',
  grid: '<rect x="3" y="3" width="7" height="7" rx="1"/><rect x="14" y="3" width="7" height="7" rx="1"/><rect x="3" y="14" width="7" height="7" rx="1"/><rect x="14" y="14" width="7" height="7" rx="1"/>',
  sliders: '<path d="M4 6h10M18 6h2M4 12h4M12 12h8M4 18h12M20 18h0"/><circle cx="16" cy="6" r="2"/><circle cx="10" cy="12" r="2"/><circle cx="18" cy="18" r="2"/>',
  keyboard: '<rect x="2" y="6" width="20" height="12" rx="2"/><path d="M6 10h.01M10 10h.01M14 10h.01M18 10h.01M7 14h10"/>',
  power: '<path d="M12 3v9"/><path d="M6.3 6.3a8 8 0 1 0 11.4 0"/>',
  battery: '<rect x="2" y="7" width="17" height="10" rx="2"/><path d="M22 11v2M6 10v4M10 10v4"/>',
  game: '<path d="M6 8h12a4 4 0 0 1 3.9 4.8l-.9 4A2.6 2.6 0 0 1 16.2 18l-1.7-2h-5l-1.7 2A2.6 2.6 0 0 1 3 16.8l-.9-4A4 4 0 0 1 6 8z"/><path d="M8 11v3M6.5 12.5h3M15 12h.01M17.5 13.5h.01"/>',
  bell: '<path d="M6 8a6 6 0 0 1 12 0c0 7 3 8 3 8H3s3-1 3-8"/><path d="M10 20a2 2 0 0 0 4 0"/>',
  users: '<circle cx="9" cy="8" r="3.5"/><path d="M2.5 20a6.5 6.5 0 0 1 13 0"/><path d="M16 4.5a3.5 3.5 0 0 1 0 7M21.5 20a6.5 6.5 0 0 0-4-6"/>',
  server: '<rect x="3" y="3" width="18" height="7" rx="2"/><rect x="3" y="14" width="18" height="7" rx="2"/><path d="M7 6.5h.01M7 17.5h.01"/>',
  hash: '<path d="M5 9h15M4 15h15M10 3 8 21M16 3l-2 18"/>',
  film: '<rect x="3" y="3" width="18" height="18" rx="2"/><path d="M7 3v18M17 3v18M3 8h4M3 16h4M17 8h4M17 16h4M3 12h18"/>',
  music: '<path d="M9 18V5l12-2v13"/><circle cx="6" cy="18" r="3"/><circle cx="18" cy="16" r="3"/>',
  archive: '<rect x="3" y="4" width="18" height="5" rx="1"/><path d="M5 9v10a2 2 0 0 0 2 2h10a2 2 0 0 0 2-2V9M10 13h4"/>',
  app: '<rect x="3" y="3" width="18" height="18" rx="4"/><path d="M8 12h8M12 8v8"/>',
  doc: '<path d="M14 3H7a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h10a2 2 0 0 0 2-2V8z"/><path d="M14 3v5h5M9 13h6M9 17h6"/>',
  disc: '<circle cx="12" cy="12" r="9"/><circle cx="12" cy="12" r="2.5"/>',
  subtitles: '<rect x="2" y="5" width="20" height="14" rx="2"/><path d="M6 13h4M14 13h4M6 16h8M16 16h2"/>',
  volume: '<path d="M4 9v6h4l5 4V5L8 9z"/><path d="M16 9a4 4 0 0 1 0 6M19 6a8 8 0 0 1 0 12"/>',
  maximize: '<path d="M4 9V4h5M20 9V4h-5M4 15v5h5M20 15v5h-5"/>',
  minimize: '<path d="M5 12h14"/>',
  winMax: '<rect x="5" y="5" width="14" height="14" rx="1"/>',
  pin: '<path d="M9 3h6l-1 6 4 3v2H6v-2l4-3z"/><path d="M12 14v7"/>',
  puzzle: '<path d="M10 3h4v3a2 2 0 1 0 4 0V3h3v7h-3a2 2 0 1 0 0 4h3v7h-7v-3a2 2 0 1 0-4 0v3H3v-7h3a2 2 0 1 0 0-4H3V3z"/>',
  terminal: '<rect x="3" y="4" width="18" height="16" rx="2"/><path d="m7 9 3 3-3 3M13 15h4"/>',
  bug: '<rect x="7" y="7" width="10" height="13" rx="5"/><path d="M12 11v9M7 13H3M21 13h-4M8 8 5 5M16 8l3-3M7 18l-3 2M17 18l3 2M9 4l1 3M15 4l-1 3"/>',
  external: '<path d="M14 4h6v6M20 4l-9 9"/><path d="M18 14v4a2 2 0 0 1-2 2H6a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h4"/>',
  lock: '<rect x="4" y="11" width="16" height="10" rx="2"/><path d="M8 11V7a4 4 0 0 1 8 0v4"/>',
  unlock: '<rect x="4" y="11" width="16" height="10" rx="2"/><path d="M8 11V7a4 4 0 0 1 7.5-2"/>',
  sort: '<path d="M7 4v16M3 16l4 4 4-4M17 20V4M13 8l4-4 4 4"/>',
  star: '<path d="m12 3 2.8 5.7 6.2.9-4.5 4.4 1 6.2L12 17.3 6.5 20.2l1-6.2L3 9.6l6.2-.9z"/>',
  cmd: '<path d="M9 6a3 3 0 1 0-3 3h12a3 3 0 1 0-3-3v12a3 3 0 1 0 3-3H6a3 3 0 1 0 3 3z"/>',
  undo: '<path d="M9 14 4 9l5-5"/><path d="M4 9h10a6 6 0 0 1 0 12h-3"/>',
  ban: '<circle cx="12" cy="12" r="9"/><path d="m5.6 5.6 12.8 12.8"/>',
  plug: '<path d="M9 2v6M15 2v6M6 8h12v3a6 6 0 0 1-12 0zM12 17v5"/>',
  move: '<path d="M5 9 2 12l3 3M9 5l3-3 3 3M15 19l-3 3-3-3M19 9l3 3-3 3M2 12h20M12 2v20"/>',
  edit: '<path d="M4 20h4L19 9l-4-4L4 16z"/><path d="m13.5 6.5 4 4"/>',
  wand: '<path d="m15 4 5 5L8 21l-5-5z"/><path d="M14 2v2M21 9h2M19 3l1.5-1.5M2 4h2M4 2v2"/>',
  stream: '<rect x="2" y="4" width="20" height="14" rx="2"/><path d="m10 8 5 3-5 3z"/><path d="M8 21h8"/>',
  sparkle: '<path d="M12 3l1.8 5.2L19 10l-5.2 1.8L12 17l-1.8-5.2L5 10l5.2-1.8z"/>',
  window: '<rect x="3" y="4" width="18" height="16" rx="2"/><path d="M3 9h18"/>',
  mini: '<rect x="3" y="4" width="18" height="16" rx="2"/><rect x="12" y="12" width="7" height="6" rx="1"/>',
  speed: '<path d="M12 14l4-4"/><path d="M3.5 18a10 10 0 1 1 17 0"/>',
  tree: '<path d="M4 4h6v4H4zM14 10h6v4h-6zM14 17h6v4h-6zM7 8v11h7M7 12h7"/>',
  seeds: '<path d="M7 20c0-6 3-10 10-12-1 7-5 10-10 10"/><path d="M4 21c2-3 5-6 8-8"/>',
  home: '<path d="M3 11 12 3l9 8v9a1 1 0 0 1-1 1h-5v-6H9v6H4a1 1 0 0 1-1-1z"/>',
  dot3v: '<circle cx="12" cy="5" r="1.2"/><circle cx="12" cy="12" r="1.2"/><circle cx="12" cy="19" r="1.2"/>',
  key: '<circle cx="8" cy="15" r="4"/><path d="m11 12 9-9M17 6l3 3M14 9l2 2"/>',
  vpn: '<path d="M12 3 4 6v6c0 5 3.5 8 8 9 4.5-1 8-4 8-9V6z"/><path d="M9 12h6M12 9v6"/>',
  ext: '<path d="M10 3h4v3a2 2 0 1 0 4 0V3h3v7h-3a2 2 0 1 0 0 4h3v7H3V3z"/>',
  scan: '<path d="M3 7V4h3M21 7V4h-3M3 17v3h3M21 17v3h-3M7 12h10"/>',
  zap: '<path d="M13 2 4 14h7l-1 8 9-12h-7z"/>',
};
const svg = (n, cls = '') => `<svg class="ico i-${n} ${cls}" viewBox="0 0 24 24" aria-hidden="true">${I[n] || ''}</svg>`;
window.ICON = I; window.ico = svg;
// The Rift mark: a slim disc cut once on a diagonal, halves slid apart. Left = currentColor, right = accent.
const LOGO = '<svg class="logo-mark" viewBox="13 0 38 64" aria-hidden="true"><path class="lh" transform="translate(-1.1 -4.6)" d="M39.24 6.66A17 28 0 1 0 24.76 57.34Z"/><path class="rh" transform="translate(1.1 4.6)" d="M39.24 6.66A17 28 0 1 1 24.76 57.34Z"/></svg>';
window.LOGO = LOGO;

/* Expand <i data-i="name" class="s|l|xl"></i> into icons */
function hydrate(root = document) {
  root.querySelectorAll('i[data-i]').forEach((el) => { el.outerHTML = svg(el.dataset.i, el.className); });
}
window.hydrate = hydrate;

/* ---------- Prototype shell ---------- */
const store = { get: (k) => { try { return localStorage.getItem(k); } catch { return null; } }, set: (k, v) => { try { localStorage.setItem(k, v); } catch {} } };
const root = document.documentElement;
const PAGES = [['index.html', 'System'], ['main.html', 'Main window'], ['dialogs.html', 'Dialogs'], ['settings.html', 'Settings'], ['tabs.html', 'Search · RSS · Stats · Logs'], ['more.html', 'Wizard · Player · Tray · Extension'], ['logo.html', 'Logo']];
// [light, dark] — each accent is tuned per theme rather than lightened mechanically
const ACC = { cobalt: ['#1E46D8', '#4C7DFF'], champagne: ['#8E6119', '#D6B072'], jade: ['#1C7A58', '#4CC89A'], teal: ['#1F7F86', '#5EC4C4'], orchid: ['#7F4BB3', '#C38BEA'], ember: ['#B4532A', '#EE8A5E'] };
function shell() {
  const here = location.pathname.split('/').pop() || 'index.html';
  const bar = document.createElement('header');
  bar.className = 'pz';
  bar.innerHTML = `<a class="brand" href="index.html">${LOGO}Rift <span class="t3" style="font-weight:500">app design</span></a>
    <nav>${PAGES.map(([h, t]) => `<a href="${h}" ${h === here ? 'aria-current="page"' : ''}>${t}</a>`).join('')}</nav>
    <div class="ctl">
      <div class="seg" role="group" aria-label="Theme"><button data-theme-set="stone">${svg('sun', 's')}Stone</button><button data-theme-set="obsidian">${svg('moon', 's')}Obsidian</button></div>
      <div class="seg" role="group" aria-label="Density"><button data-density-set="compact">Compact</button><button data-density-set="comfortable">Comfortable</button></div>
      <div class="acc" role="group" aria-label="Accent">${Object.entries(ACC).map(([k, v]) => `<button style="--c:${v[1]}" data-acc="${k}" title="Accent: ${k}" aria-label="Accent ${k}"></button>`).join('')}</div>
    </div>`;
  document.body.prepend(bar);
  const sysDark = matchMedia('(prefers-color-scheme: dark)').matches;
  const apply = () => {
    const t = store.get('rift_app_theme') || (sysDark ? 'obsidian' : 'stone');
    const d = store.get('rift_app_density') || 'compact';
    let a = store.get('rift_app_accent'); if (!ACC[a]) a = 'cobalt';
    root.dataset.theme = t; root.dataset.density = d;
    const dk = t === 'obsidian', base = ACC[a][dk ? 1 : 0];
    root.style.setProperty('--accent', base);
    root.style.setProperty('--accent-hover', dk ? mix(base, '#ffffff', .14) : mix(base, '#000000', .12));
    root.style.setProperty('--accent-pressed', mix(base, '#000000', dk ? .12 : .24));
    root.style.setProperty('--piece-have', base);
    root.style.setProperty('--on-accent', lum(base) > .45 ? '#0B0C10' : '#FFFFFF');
    bar.querySelectorAll('[data-theme-set]').forEach((b) => b.setAttribute('aria-pressed', String(b.dataset.themeSet === t)));
    bar.querySelectorAll('[data-density-set]').forEach((b) => b.setAttribute('aria-pressed', String(b.dataset.densitySet === d)));
    bar.querySelectorAll('[data-acc]').forEach((b) => b.setAttribute('aria-pressed', String(b.dataset.acc === a)));
    document.dispatchEvent(new CustomEvent('themechange'));
  };
  bar.addEventListener('click', (e) => {
    const b = e.target.closest('button'); if (!b) return;
    if (b.dataset.themeSet) store.set('rift_app_theme', b.dataset.themeSet);
    if (b.dataset.densitySet) store.set('rift_app_density', b.dataset.densitySet);
    if (b.dataset.acc) store.set('rift_app_accent', b.dataset.acc);
    apply();
  });
  apply();
  // Screen index
  const idx = document.querySelector('.pidx');
  if (idx) {
    const scrs = [...document.querySelectorAll('.scr')];
    idx.innerHTML = scrs.map((s) => `<a href="#${s.id}">${s.dataset.t || s.querySelector('h2')?.textContent}</a>`).join('');
    const links = [...idx.querySelectorAll('a')];
    const io = new IntersectionObserver((es) => es.forEach((e) => {
      if (e.isIntersecting) links.forEach((l) => l.classList.toggle('on', l.getAttribute('href') === '#' + e.target.id));
    }), { rootMargin: '-30% 0px -60% 0px' });
    scrs.forEach((s) => io.observe(s));
  }
}
// Relative luminance (0–1), used to pick readable text on the accent
function lum(h) { const v = [1, 3, 5].map((i) => parseInt(h.slice(i, i + 2), 16) / 255).map((c) => c <= .03928 ? c / 12.92 : ((c + .055) / 1.055) ** 2.4); return .2126 * v[0] + .7152 * v[1] + .0722 * v[2]; }
function mix(a, b, t) {
  const p = (h) => [1, 3, 5].map((i) => parseInt(h.slice(i, i + 2), 16));
  const [x, y] = [p(a), p(b)];
  return '#' + x.map((v, i) => Math.round(v + (y[i] - v) * t).toString(16).padStart(2, '0')).join('');
}

/* ---------- Generic behaviors ---------- */
function behaviors(scope = document) {
  // Tabs: <div class="tabs" data-tabs="group"> <button class="tab" data-tab="x"> ; panes [data-pane="group:x"]
  scope.querySelectorAll('[data-tabs]').forEach((tabs) => {
    if (tabs._b) return; tabs._b = 1;
    const g = tabs.dataset.tabs;
    tabs.setAttribute('role', 'tablist');
    tabs.addEventListener('click', (e) => {
      const t = e.target.closest('[data-tab]'); if (!t) return;
      tabs.querySelectorAll('[data-tab]').forEach((x) => x.setAttribute('aria-selected', String(x === t)));
      document.querySelectorAll(`[data-pane^="${g}:"]`).forEach((p) => { p.hidden = p.dataset.pane !== `${g}:${t.dataset.tab}`; });
      tabs.dispatchEvent(new CustomEvent('tabchange', { detail: t.dataset.tab, bubbles: true }));
    });
  });
  // Segmented: single-select
  scope.querySelectorAll('.seg:not([data-free])').forEach((s) => {
    if (s._b || s.closest('.pz')) return; s._b = 1;
    s.addEventListener('click', (e) => {
      const b = e.target.closest('button'); if (!b) return;
      s.querySelectorAll('button').forEach((x) => x.setAttribute('aria-pressed', String(x === b)));
    });
  });
  // Toggle buttons
  scope.querySelectorAll('[data-toggle]').forEach((b) => {
    if (b._b) return; b._b = 1;
    b.addEventListener('click', () => b.setAttribute('aria-pressed', String(b.getAttribute('aria-pressed') !== 'true')));
  });
  // Ranges paint their fill
  scope.querySelectorAll('.range').forEach((r) => {
    const paint = () => r.style.setProperty('--v', ((r.value - r.min) / (r.max - r.min) * 100) + '%');
    paint(); r.addEventListener('input', paint);
  });
  // Indeterminate checkboxes
  scope.querySelectorAll('input[data-indet]').forEach((c) => { c.indeterminate = true; });
  // Scheduler cells cycle Normal → Alternative → Paused
  scope.querySelectorAll('.sched i').forEach((c) => {
    if (c._b) return; c._b = 1;
    c.addEventListener('click', () => { c.className = c.classList.contains('alt') ? 'off' : c.classList.contains('off') ? '' : 'alt'; });
  });
}
window.behaviors = behaviors;

/* ---------- Helpers shared by pages ---------- */
window.fmtBytes = (mb) => mb >= 1024 ? (mb / 1024).toFixed(mb >= 10240 ? 1 : 2) + ' GB' : mb >= 1 ? mb.toFixed(mb >= 100 ? 0 : 1) + ' MB' : (mb * 1024).toFixed(0) + ' KB';
window.fmtSpeed = (v) => v <= 0 ? '0 B/s' : v >= 1 ? v.toFixed(1) + ' MB/s' : (v * 1024).toFixed(0) + ' KB/s';
window.fmtEta = (s) => { if (!isFinite(s) || s <= 0) return '∞'; if (s < 60) return Math.round(s) + 's'; const m = Math.floor(s / 60); if (m < 60) return m + 'm ' + String(Math.round(s % 60)).padStart(2, '0') + 's'; const h = Math.floor(m / 60); if (h < 48) return h + 'h ' + String(m % 60).padStart(2, '0') + 'm'; return Math.floor(h / 24) + 'd ' + (h % 24) + 'h'; };
window.drawGraph = (cv, series, opts = {}) => {
  const d = Math.min(devicePixelRatio || 1, 2), w = cv.clientWidth, h = cv.clientHeight;
  if (!w || !h) return;
  cv.width = w * d; cv.height = h * d; const c = cv.getContext('2d'); c.setTransform(d, 0, 0, d, 0, 0);
  const cs = getComputedStyle(document.documentElement);
  const col = (n) => cs.getPropertyValue(n).trim() || n;
  const max = opts.max || Math.max(1, ...series.flatMap((s) => s.data)) * 1.15;
  c.strokeStyle = col('--line-soft'); c.lineWidth = 1;
  for (let i = 1; i < 4; i++) { const y = Math.round(h * i / 4) + .5; c.beginPath(); c.moveTo(0, y); c.lineTo(w, y); c.stroke(); }
  if (opts.limit) { const y = h - opts.limit / max * h; c.setLineDash([4, 4]); c.strokeStyle = col('--text3'); c.beginPath(); c.moveTo(0, y); c.lineTo(w, y); c.stroke(); c.setLineDash([]); }
  series.forEach((s) => {
    const n = s.data.length; if (n < 2) return;
    const X = (i) => i / (n - 1) * w, Y = (v) => h - 2 - v / max * (h - 4);
    c.beginPath(); s.data.forEach((v, i) => (i ? c.lineTo(X(i), Y(v)) : c.moveTo(X(i), Y(v))));
    c.strokeStyle = col(s.color); c.lineWidth = s.width || 1.8; c.lineJoin = 'round'; c.stroke();
    if (s.fill) { c.lineTo(w, h); c.lineTo(0, h); c.closePath(); c.globalAlpha = .12; c.fillStyle = col(s.color); c.fill(); c.globalAlpha = 1; }
  });
};

document.addEventListener('DOMContentLoaded', () => { hydrate(); shell(); behaviors(); });
})();

/* ---------- Tooltips: every interactive thing explains itself ----------
   Source order: data-tip, title (moved to data-tip so the native one never shows),
   aria-label, then the visible label. Shortcut comes from data-kbd. 450 ms delay,
   instant when moving between neighbours, also on keyboard focus. */
(() => {
  const SEL = '[data-tip],[title],[aria-label],button,.ni,.tab,.badge,.kind,.chip,.health,.flag,.statusdot,.mi,.th>span,.sw,.chk,.rad,.it,a,select,.segbar,.prog,.pbar,.dot,.sw8';
  let tip, timer, cur, warm = 0;
  const text = (el) => {
    if (el.hasAttribute('title')) { el.dataset.tip = el.getAttribute('title'); el.removeAttribute('title'); }
    let t = el.dataset.tip || el.getAttribute('aria-label') || '';
    if (!t) { t = (el.innerText || el.textContent || '').replace(/\s+/g, ' ').trim(); if (t.length > 90) t = ''; }
    return t;
  };
  const show = (el) => {
    const t = text(el); if (!t) return;
    tip = tip || Object.assign(document.createElement('div'), { className: 'rtip', role: 'tooltip' });
    if (!tip.isConnected) document.body.append(tip);
    tip.innerHTML = t.replace(/&/g, '&amp;').replace(/</g, '&lt;') + (el.dataset.kbd ? `<span class="kbd">${el.dataset.kbd}</span>` : '');
    const r = el.getBoundingClientRect(), w = tip.offsetWidth, h = tip.offsetHeight;
    let x = Math.max(8, Math.min(innerWidth - w - 8, r.left + r.width / 2 - w / 2)), y = r.bottom + 8;
    if (y + h > innerHeight - 8) y = r.top - h - 8;
    tip.style.left = x + 'px'; tip.style.top = y + 'px';
    requestAnimationFrame(() => tip.classList.add('on')); warm = Date.now();
  };
  const hide = () => { clearTimeout(timer); cur = null; tip && tip.classList.remove('on'); };
  document.addEventListener('mouseover', (e) => {
    const el = e.target.closest(SEL); if (el === cur) return; hide(); if (!el || el.closest('.rtip,.pz')) return;
    cur = el; timer = setTimeout(() => show(el), Date.now() - warm < 600 ? 60 : 450);
  });
  document.addEventListener('mousedown', hide, true); document.addEventListener('scroll', hide, true);
  document.addEventListener('focusin', (e) => { if (e.target.matches(':focus-visible')) { const el = e.target.closest(SEL); if (el) { cur = el; timer = setTimeout(() => show(el), 300); } } });
  document.addEventListener('focusout', hide);
})();

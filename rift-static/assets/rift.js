/* Rift site v3 — shared behavior (landing + download) */
(() => {
  const $ = (s, r = document) => r.querySelector(s);
  const $$ = (s, r = document) => [...r.querySelectorAll(s)];
  const RM = matchMedia('(prefers-reduced-motion: reduce)').matches;
  const FINE = matchMedia('(hover:hover) and (pointer:fine)').matches;
  const store = {
    get(k) { try { return localStorage.getItem(k); } catch { return null; } },
    set(k, v) { try { localStorage.setItem(k, v); } catch {} },
  };

  /* ---------- Theme ---------- */
  const sun = '<circle cx="12" cy="12" r="4.5"/><path d="M12 2v2.5M12 19.5V22M4.2 4.2l1.8 1.8M18 18l1.8 1.8M2 12h2.5M19.5 12H22M4.2 19.8 6 18M18 6l1.8-1.8"/>';
  const moon = '<path d="M20.5 13.2A8.5 8.5 0 1 1 10.8 3.5a6.6 6.6 0 0 0 9.7 9.7z"/>';
  function curTheme() {
    const t = document.documentElement.dataset.theme;
    if (t) return t;
    return matchMedia('(prefers-color-scheme: dark)').matches ? 'obsidian' : 'stone';
  }
  function paintThemeIcon() {
    const ic = $('#themeIcon');
    if (ic) ic.innerHTML = curTheme() === 'obsidian' ? moon : sun;
  }
  const saved = store.get('rift_theme');
  if (saved === 'stone' || saved === 'obsidian') document.documentElement.dataset.theme = saved;
  paintThemeIcon();
  $('#themeToggle')?.addEventListener('click', () => {
    const next = curTheme() === 'obsidian' ? 'stone' : 'obsidian';
    document.documentElement.dataset.theme = next;
    store.set('rift_theme', next);
    paintThemeIcon();
  });

  /* ---------- Split text into words/chars ---------- */
  function split(el) {
    if (el.dataset.done) return;
    el.dataset.done = '1';
    const label = el.textContent.replace(/\s+/g, ' ').trim();
    if (!el.hasAttribute('aria-label') && !el.closest('[aria-hidden="true"]')) el.setAttribute('aria-label', label);
    let i = +(el.dataset.offset || 0);
    const walk = (node, out) => {
      node.childNodes.forEach((n) => {
        if (n.nodeType === 3) {
          n.textContent.split(/(\s+)/).forEach((part) => {
            if (!part) return;
            if (/^\s+$/.test(part)) { out.append(' '); return; }
            const w = document.createElement('span');
            w.className = 'w'; w.setAttribute('aria-hidden', 'true');
            [...part].forEach((c) => {
              const s = document.createElement('span');
              s.className = 'ch'; s.textContent = c; s.style.setProperty('--i', i++);
              w.append(s);
            });
            out.append(w);
          });
        } else if (n.nodeType === 1) {
          const clone = n.cloneNode(false);
          walk(n, clone);
          out.append(clone);
        }
      });
    };
    const frag = document.createDocumentFragment();
    walk(el, frag);
    el.textContent = '';
    el.append(frag);
  }
  $$('[data-split]').forEach(split);

  /* ---------- Reveal on scroll ---------- */
  const io = new IntersectionObserver((es) => {
    es.forEach((e) => { if (e.isIntersecting) { e.target.classList.add('in'); io.unobserve(e.target); } });
  }, { rootMargin: '0px 0px -8% 0px', threshold: 0.12 });
  $$('[data-split],.rv').forEach((el) => { if (!el.hasAttribute('data-manual')) io.observe(el); });

  /* ---------- Kinetic proximity (variable font reacts to the cursor) ---------- */
  let px = -9999, py = -9999;
  addEventListener('pointermove', (e) => { px = e.clientX; py = e.clientY; }, { passive: true });
  const kinSets = $$('[data-kin]').map((el) => ({
    el, chars: $$('.ch', el),
    base: (el.dataset.kin || '600,82').split(',').map(Number),
    peak: (el.dataset.peak || '800,100').split(',').map(Number),
    r: +(el.dataset.radius || 260), vis: false,
  }));
  const kio = new IntersectionObserver((es) => es.forEach((e) => {
    const k = kinSets.find((s) => s.el === e.target); if (k) k.vis = e.isIntersecting;
  }));
  kinSets.forEach((k) => kio.observe(k.el));
  function kinTick() {
    if (FINE && !RM) {
      for (const k of kinSets) {
        if (!k.vis) continue;
        for (const c of k.chars) {
          const r = c.getBoundingClientRect();
          const d = Math.hypot(px - (r.left + r.width / 2), py - (r.top + r.height / 2));
          const f = Math.max(0, 1 - d / k.r);
          const e = f * f * (3 - 2 * f);
          const w = Math.round(k.base[0] + (k.peak[0] - k.base[0]) * e);
          const wd = Math.round(k.base[1] + (k.peak[1] - k.base[1]) * e);
          const key = w + '|' + wd;
          if (c._k !== key) { c._k = key; c.style.fontVariationSettings = `"wght" ${w},"wdth" ${wd},"opsz" 96`; }
        }
      }
    }
    requestAnimationFrame(kinTick);
  }
  requestAnimationFrame(kinTick);

  /* ---------- Custom cursor ---------- */
  if (FINE && !RM) {
    const cur = document.createElement('div');
    cur.className = 'cur'; cur.setAttribute('aria-hidden', 'true');
    document.body.append(cur);
    let cx = 0, cy = 0;
    addEventListener('pointermove', (e) => {
      cur.classList.add('on');
      const t = e.target.closest?.('a,button,[role="tab"],.slab,input,label');
      cur.classList.toggle('link', !!t);
    }, { passive: true });
    document.addEventListener('pointerleave', () => cur.classList.remove('on'));
    (function loop() {
      cx += (px - cx) * 0.22; cy += (py - cy) * 0.22;
      cur.style.transform = `translate3d(${cx}px,${cy}px,0)`;
      requestAnimationFrame(loop);
    })();
  }

  /* ---------- Nav: solid after scroll, hide on scroll down ---------- */
  const nav = $('.nav');
  let lastY = scrollY;
  addEventListener('scroll', () => {
    const y = scrollY;
    nav?.classList.toggle('solid', y > 40);
    nav?.classList.toggle('hide', y > 400 && y > lastY + 4);
    if (y < lastY - 4) nav?.classList.remove('hide');
    lastY = y;
  }, { passive: true });

  /* ---------- Marquee with scroll-velocity skew ---------- */
  $$('.mq').forEach((mq) => {
    const tr = $('.mq-track', mq);
    if (!tr) return;
    tr.innerHTML += tr.innerHTML; // duplicate for seamless loop
    let x = 0, v = 0, prev = scrollY;
    (function loop() {
      const dy = scrollY - prev; prev = scrollY;
      v += (dy - v) * 0.1;
      if (!RM) x -= 0.6 + Math.abs(v) * 0.25;
      const half = tr.scrollWidth / 2;
      if (half && -x > half) x += half;
      tr.style.transform = `translate3d(${x}px,0,0) skewX(${Math.max(-14, Math.min(14, -v * 0.6))}deg)`;
      requestAnimationFrame(loop);
    })();
  });

  /* ---------- Big end word: gradient follows cursor ---------- */
  $$('.big').forEach((b) => {
    const spans = $$('span', b);
    const sync = () => {
      const br = b.getBoundingClientRect();
      spans.forEach((s) => {
        const r = s.getBoundingClientRect();
        s.style.setProperty('--bw', br.width + 'px');
        s.style.setProperty('--bx', (br.left - r.left) + 'px');
      });
    };
    sync(); addEventListener('resize', sync);
    addEventListener('pointermove', (e) => {
      const r = b.getBoundingClientRect();
      spans.forEach((s) => { s.style.setProperty('--mx', (e.clientX - r.left) + 'px'); s.style.setProperty('--my', (e.clientY - r.top) + 'px'); });
    }, { passive: true });
  });

  /* ---------- Toast ---------- */
  let tt;
  function toast(msg) {
    const t = $('#toast'); if (!t) return;
    t.textContent = msg; t.classList.add('on');
    clearTimeout(tt); tt = setTimeout(() => t.classList.remove('on'), 2600);
  }

  /* ---------- Tear page transition ---------- */
  const tear = $('#tear');
  function go(url) {
    if (RM || !tear) { location.href = url; return; }
    tear.classList.add('shut');
    try { sessionStorage.setItem('rift_tear', '1'); } catch {}
    setTimeout(() => { location.href = url; }, 620);
  }
  document.addEventListener('click', (e) => {
    const a = e.target.closest('a[data-go]');
    if (!a || e.metaKey || e.ctrlKey || e.shiftKey || e.button) return;
    const href = a.getAttribute('href');
    if (!href || href.startsWith('#')) return;
    e.preventDefault(); go(href);
  });
  let arrived = false;
  try { arrived = sessionStorage.getItem('rift_tear') === '1'; sessionStorage.removeItem('rift_tear'); } catch {}
  if (tear && arrived && !RM) {
    tear.classList.add('instant', 'shut');
    requestAnimationFrame(() => requestAnimationFrame(() => { tear.classList.remove('instant'); tear.classList.remove('shut'); }));
  }
  addEventListener('pageshow', (e) => { if (e.persisted) tear?.classList.remove('shut'); });

  /* ---------- OS detection ---------- */
  function detectOS() {
    const ua = navigator.userAgent, p = (navigator.userAgentData?.platform || navigator.platform || '').toLowerCase();
    let os = 'windows';
    if (/mac/.test(p) || /Mac OS X/.test(ua)) os = 'macos';
    else if (/linux|x11/.test(p) && !/android/i.test(ua)) os = 'linux';
    else if (/android|iphone|ipad/i.test(ua)) os = 'mobile';
    const arm = /arm|aarch64/i.test(ua + ' ' + p);
    return { os, arm };
  }

  window.Rift = { $, $$, RM, FINE, toast, go, split, detectOS, store, pointer: () => ({ x: px, y: py }) };
})();

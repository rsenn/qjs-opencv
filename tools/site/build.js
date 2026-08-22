/**
 * Static-site generator for the qjs-opencv GitHub Pages site.
 *
 *   qjsm tools/site/build.js [outdir]      (default outdir: _site)
 *
 * Renders the repo's own markdown into a self-contained HTML site: a
 * hand-written landing page plus every doc page behind a shared shell with
 * sidebar navigation and a per-page table of contents. Inter-doc *.md links
 * are rewritten to their generated pages; links pointing at anything else in
 * the repo (js_*.cpp, include/, examples/) become github.com blob/tree URLs.
 *
 * Same shape as qjs-lws' tools/site - markdown.js and highlight.js are shared
 * verbatim between the two projects.
 *
 * Everything is relative-path linked so the site works both at
 * https://rsenn.github.io/qjs-opencv/ and from a local file:// checkout.
 */

import * as std from 'std';
import * as os from 'os';
import { render } from './markdown.js';
import { highlight } from './highlight.js';

const REPO = 'rsenn/qjs-opencv';
const GITHUB = 'https://github.com/' + REPO;
const TAGLINE = 'OpenCV bindings for QuickJS';

/* --------------------------------------------------------------- site map */

const NAV = [
  {
    group: 'Start here',
    pages: [
      ['README.md', 'getting-started.html', 'Getting started'],
    ],
  },
  {
    group: 'Reference',
    pages: [
      ['doc/algorithms.md', 'docs/algorithms.html', 'Custom algorithms'],
      ['TODO.md', 'docs/roadmap.html', 'Binding roadmap'],
    ],
  },
];

/* doc/opencv-js-api.md and doc/opencv-js-examples.md are deliberately absent:
 * they document upstream opencv.js (the porting target), not this project,
 * and cite local checkout paths. */

const PAGES = [];
for(const { group, pages } of NAV)
  for(const [src, out, title] of pages) PAGES.push({ src, out, title, group });

const bySrc = new Map(PAGES.map(p => [p.src, p]));

/* ------------------------------------------------------------ path helpers */

const dirname = p => (p.indexOf('/') >= 0 ? p.slice(0, p.lastIndexOf('/')) : '');

function normalize(p) {
  const out = [];
  for(const part of p.split('/')) {
    if(!part || part === '.') continue;
    if(part === '..') out.pop();
    else out.push(part);
  }
  return out.join('/');
}

/** Path from a directory to a file, both site-relative. */
function relative(fromDir, to) {
  const f = fromDir ? fromDir.split('/') : [];
  const t = to.split('/');
  let i = 0;
  while(i < f.length && i < t.length - 1 && f[i] === t[i]) i++;
  return '../'.repeat(f.length - i) + t.slice(i).join('/');
}

function mkdirp(path) {
  let cur = path.startsWith('/') ? '' : '.';
  for(const part of path.split('/')) {
    if(!part) continue;
    cur += '/' + part;
    os.mkdir(cur, 0o755);
  }
}

function read(path) {
  const text = std.loadFile(path);
  if(text === null) throw new Error('cannot read ' + path);
  return text;
}

function write(path, text) {
  mkdirp(dirname(path));
  const f = std.open(path, 'w');
  if(!f) throw new Error('cannot write ' + path);
  f.puts(text);
  f.close();
}

/* --------------------------------------------------------- link rewriting */

/** Rewrite one markdown href found in `page` into a link that works on the site. */
function linkFor(page, href) {
  if(!href || href.startsWith('#') || href.startsWith('//') || /^[a-z][a-z0-9+.-]*:/i.test(href))
    return href;

  const hash = href.indexOf('#');
  const path = hash < 0 ? href : href.slice(0, hash);
  const frag = hash < 0 ? '' : href.slice(hash);
  if(!path) return href;

  const target = normalize(dirname(page.src) + '/' + path);
  const hit = bySrc.get(target);
  if(hit) return relative(dirname(page.out), hit.out) + frag;

  const kind = path.endsWith('/') ? 'tree' : 'blob';
  return GITHUB + '/' + kind + '/main/' + target + frag;
}

/* ------------------------------------------------------------- html shell */

const escAttr = s => s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/"/g, '&quot;');

function sidebar(page) {
  const here = dirname(page.out);
  let html = '';
  for(const { group, pages } of NAV) {
    html += '<div class="navgroup"><h3>' + group + '</h3><ul>';
    for(const [, out, title] of pages) {
      const cls = out === page.out ? ' class="here"' : '';
      html += '<li><a href="' + escAttr(relative(here, out)) + '"' + cls + '>' + title + '</a></li>';
    }
    html += '</ul></div>';
  }
  return html;
}

function toc(headings) {
  const items = headings.filter(h => h.level >= 2 && h.level <= 3);
  if(items.length < 2) return '';
  const links = items.map(h =>
    '<li class="lvl' + h.level + '"><a href="#' + escAttr(h.id) + '">' + escAttr(h.text) + '</a></li>').join('');
  return '<nav class="toc"><h3>On this page</h3><ul>' + links + '</ul></nav>';
}

function shell({ title, root, body, cls }) {
  return `<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>${escAttr(title)}</title>
<meta name="description" content="${escAttr(TAGLINE)} — Mat, contours, imgproc, calib3d, dnn and video I/O in a loadable QuickJS module.">
<link rel="stylesheet" href="${root}assets/style.css">
<link rel="icon" href="${root}assets/favicon.svg" type="image/svg+xml">
<script>try{var t=localStorage.getItem('theme');if(t)document.documentElement.dataset.theme=t}catch(e){}</script>
</head>
<body class="${cls}">
<header class="topbar">
  <a class="brand" href="${root}index.html"><span class="mark">{ }</span> qjs-opencv</a>
  <nav class="topnav">
    <a href="${root}getting-started.html">Get started</a>
    <a href="${root}docs/algorithms.html">Docs</a>
    <a href="${root}docs/roadmap.html">Roadmap</a>
    <a href="${GITHUB}" target="_blank" rel="noopener">GitHub</a>
  </nav>
  <button class="themetoggle" type="button" aria-label="Toggle colour scheme">◐</button>
</header>
${body}
<footer class="sitefoot">
  <p>qjs-opencv — MIT licensed. Built from the repo's own markdown by
     <a href="${GITHUB}/blob/main/tools/site/build.js">tools/site/build.js</a>, running on qjsm.</p>
</footer>
<script>
document.querySelector('.themetoggle').addEventListener('click', function () {
  var d = document.documentElement;
  var dark = d.dataset.theme ? d.dataset.theme === 'dark'
    : matchMedia('(prefers-color-scheme: dark)').matches;
  d.dataset.theme = dark ? 'light' : 'dark';
  try { localStorage.setItem('theme', d.dataset.theme); } catch (e) {}
});
</script>
</body>
</html>
`;
}

/* ------------------------------------------------------------------ build */

function buildPage(page) {
  const md = read(page.src);
  const { html, headings } = render(md, {
    link: href => linkFor(page, href),
    highlight,
  });

  const depth = page.out.split('/').length - 1;
  const root = '../'.repeat(depth);
  const h1 = headings.find(h => h.level === 1);

  const body = `<div class="layout">
<aside class="sidebar">${sidebar(page)}</aside>
<main class="doc">
<article>${html}</article>
<p class="editlink"><a href="${GITHUB}/blob/main/${page.src}">Edit this page on GitHub →</a></p>
</main>
${toc(headings)}
</div>`;

  write(OUT + '/' + page.out, shell({
    title: (h1 ? h1.text : page.title) + ' — qjs-opencv',
    root, body, cls: 'has-sidebar',
  }));
}

/** Strip the common leading indentation from a block of source text. */
function dedent(text) {
  const lines = text.replace(/^\n+|\s+$/g, '').split('\n');
  const pad = Math.min(...lines.filter(l => l.trim()).map(l => l.match(/^ */)[0].length));
  return lines.map(l => l.slice(pad)).join('\n');
}

/**
 * The landing page is hand-written HTML, so its code samples are marked up as
 * <x-code lang="js">…</x-code> and expanded here through the same highlighter
 * the markdown pages use.
 */
function expandCode(html) {
  return html.replace(/<x-code lang="([^"]*)">([\s\S]*?)<\/x-code>/g, (m, lang, body) => {
    const src = dedent(body).replace(/&lt;/g, '<').replace(/&gt;/g, '>').replace(/&amp;/g, '&');
    return '<div class="codeblock" data-lang="' + escAttr(lang) + '"><pre><code class="language-' +
      escAttr(lang) + '">' + highlight(src, lang) + '</code></pre></div>';
  });
}

function buildLanding() {
  // placeholders first: they also appear inside <x-code>, where expansion
  // would otherwise scatter them across highlight spans
  const body = expandCode(read(SELF + '/landing.html')
    .replace(/\{\{GITHUB\}\}/g, GITHUB)
    .replace(/\{\{REPO\}\}/g, REPO));
  write(OUT + '/index.html', shell({
    title: 'qjs-opencv — ' + TAGLINE,
    root: '', body, cls: 'landing',
  }));
}

const SELF = dirname(import.meta.url.replace(/^file:\/\//, '')) || '.';
const OUT = scriptArgs[1] || '_site';

buildLanding();
for(const page of PAGES) buildPage(page);

write(OUT + '/assets/style.css', read(SELF + '/style.css'));
write(OUT + '/assets/favicon.svg', read(SELF + '/favicon.svg'));
write(OUT + '/.nojekyll', '');

console.log('built ' + (PAGES.length + 1) + ' pages into ' + OUT + '/');

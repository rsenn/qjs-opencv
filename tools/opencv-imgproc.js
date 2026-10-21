#!/usr/bin/env qjsm
/*
 * opencv-imgproc: clip, cutout and palette, one json line on stdout
 * (quickjs only)
 *
 * options are --name=value (cv.CommandLineParser), see --help
 * spec: doc/opencv-imgproc.md, run with QUICKJS_MODULE_PATH=<build dir>
 */
import * as cv from 'opencv';
import process from 'process';

const keys = `{ help h usage ? |      | print this help }
  { @cmd                 |      | clip, cutout or palette }
  { in                   |      | input image }
  { out                  |      | output image, .png or .webp (clip, cutout) }
  { polygon              |      | clip: JSON points [[x,y],...] }
  { invert               | false| clip: keep the outside of the polygon }
  { supersample          | 4    | clip: edge supersampling, 1-8 }
  { rect                 |      | cutout: subject box x,y,w,h (default centre 80%) }
  { count                | 12   | palette: number of colors }
  { dist                 | cube | palette: cube, cie76, cie94 or kmeans }
  { hsv                  | false| palette: work in HSV instead of BGR }`;

const fail = (code, message) => {
  console.log(JSON.stringify({ ok: false, error: { code, message } }));
  process.exit(code);
};

const parser = new cv.CommandLineParser(process.argv.slice(1), keys);
parser.about('opencv-imgproc: clip a photo by a polygon, cut out its subject, get its palette\n' + 'usage: opencv-imgproc <clip|cutout|palette> --in=FILE [--out=FILE] [options]');

if(parser.has('help')) {
  parser.printMessage();
  process.exit(0);
}

const cmd = parser.get('@cmd');
const opt = n => parser.get(n) || undefined;
const flag = n => parser.has(n) && parser.get(n) != 'false';
const isPalette = cmd == 'palette';

if(!['clip', 'cutout', 'palette'].includes(cmd) || !opt('in') || (!isPalette && !opt('out'))) {
  fail(2, 'usage: clip|cutout|palette --in=FILE [--out=FILE] (see --help)');
}
if(!isPalette && !/\.(png|webp)$/i.test(opt('out'))) fail(4, 'output must be .png or .webp (needs alpha)');

let src;
try {
  src = cv.imread(opt('in'), cv.IMREAD_UNCHANGED);
} catch(e) {}
if(!src || src.empty()) fail(3, 'cannot read ' + opt('in'));

const { cols: w, rows: h } = src;
const bgr = new cv.Mat();
const alpha = new cv.Mat(h, w, cv.CV_8UC1, [255]);

if(src.channels() == 4) {
  const c = cv.split(src);
  cv.merge(c.slice(0, 3), bgr);
  c[3].copyTo(alpha);
} else if(src.channels() == 1) cv.cvtColor(src, bgr, cv.COLOR_GRAY2BGR);
else src.copyTo(bgr);

if(isPalette) {
  // work on a reduced copy, the histogram methods do not need more
  const k = Math.min(1, 512 / Math.max(w, h));
  const small = new cv.Mat();
  const dist = ['cube', 'cie76', 'cie94', 'kmeans'].indexOf(opt('dist'));
  const hsv = flag('hsv');

  if(dist < 0) fail(2, '--dist must be cube, cie76, cie94 or kmeans');

  cv.resize(bgr, small, new cv.Size(Math.round(w * k), Math.round(h * k)), 0, 0, cv.INTER_AREA);

  const colors = cv.paletteGenerate(small, (hsv ? 1 : 0) | (dist << 1), +opt('count') || 12);
  const hex = c => '#' + [2, 1, 0].map(i => c[i].toString(16).padStart(2, '0')).join('');

  console.log(JSON.stringify({ ok: true, command: cmd, space: hsv ? 'hsv' : 'bgr', input: { width: w, height: h, channels: src.channels() }, colors: colors.map(c => (hsv ? [c[0], c[1], c[2]] : hex(c))) }));
  process.exit(0);
}

let mask;

if(cmd == 'clip') {
  // supersampled fill, area-downsampled for anti-aliased edges
  const polys = JSON.parse(opt('polygon') ?? fail(2, '--polygon required'));
  const ss = Math.max(1, Math.min(8, +opt('supersample') || 4));
  const big = new cv.Mat(h * ss, w * ss, cv.CV_8UC1, [0]);
  const scale = p => [p[0] * ss, p[1] * ss];

  if(!Array.isArray(polys) || polys.length < 3) fail(6, 'polygon needs at least 3 points');
  if(!polys.flat(2).every(Number.isFinite)) fail(6, 'polygon has a non-finite coordinate');

  for(const poly of Array.isArray(polys[0][0]) ? polys : [polys]) cv.fillPoly(big, [poly.map(scale)], [255]);

  mask = new cv.Mat();
  cv.resize(big, mask, new cv.Size(w, h), 0, 0, cv.INTER_AREA);
} else {
  // grabcut at a reduced size, centre 80% box unless --rect
  const k = Math.min(1, 1280 / Math.max(w, h));
  const small = new cv.Mat();
  const gc = new cv.Mat(Math.round(h * k), Math.round(w * k), cv.CV_8UC1, [0]);
  const r = opt('rect') ? opt('rect').split(',').map(Number) : [w * 0.1, h * 0.1, w * 0.8, h * 0.8];

  cv.resize(bgr, small, new cv.Size(gc.cols, gc.rows), 0, 0, cv.INTER_AREA);
  cv.grabCut(small, gc, new cv.Rect(...r.map(v => Math.round(v * k))), new cv.Mat(), new cv.Mat(), 5, cv.GC_INIT_WITH_RECT);
  cv.bitwise_and(gc, new cv.Mat(gc.rows, gc.cols, cv.CV_8UC1, [1]), gc);
  cv.threshold(gc, gc, 0, 255, cv.THRESH_BINARY);

  mask = new cv.Mat();
  cv.resize(gc, mask, new cv.Size(w, h), 0, 0, cv.INTER_LINEAR);
  cv.GaussianBlur(mask, mask, new cv.Size(0, 0), 1);
}

if(flag('invert')) cv.bitwise_not(mask, mask);

// intersect with any alpha the input already had
cv.multiply(alpha, mask, alpha, 1 / 255);

const out = new cv.Mat();
cv.merge([...cv.split(bgr), alpha], out);

if(!cv.imwrite(opt('out'), out)) fail(10, 'cannot write ' + opt('out'));

console.log(JSON.stringify({ ok: true, command: cmd, input: { width: w, height: h, channels: src.channels() }, output: { path: opt('out'), hasAlpha: true }, mask: { coverage: cv.countNonZero(alpha) / (w * h) } }));

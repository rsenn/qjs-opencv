#!/usr/bin/env qjsm
/*
 * opencv-imgproc: cutout with a trained model, clip, palette, info and
 * capabilities. One json line on stdout, diagnostics on stderr.
 *
 * quickjs only, it needs the opencv module (QUICKJS_MODULE_PATH=<build>)
 * spec: doc/opencv-imgproc.md, usage: opencv-imgproc.js --help
 */
import * as cv from 'opencv';
import * as fs from 'fs';
import * as path from 'path';
import process from 'process';
import { performance } from 'perf_hooks';

const VERSION = '1.2.0';

const EXIT = { E_USAGE: 2, E_INPUT: 3, E_FORMAT: 4, E_TOO_LARGE: 5, E_BAD_SHAPE: 6, E_MASK_SIZE: 7, E_MODEL: 8, E_METHOD: 9, E_WRITE: 10, E_INTERNAL: 70 };

class Fail extends Error {
  constructor(code, message, detail) {
    super(message);
    this.code = code;
    this.detail = detail;
  }
}

const fail = (code, message, detail) => {
  throw new Fail(code, message, detail);
};

const now = () => performance.now();
const ms = t => Math.round(now() - t);

// every line the tool prints is one json object, stdout carries progress
// and result events, stderr carries log events
const emit = (stream, obj) => stream.write(JSON.stringify(obj) + '\n');
const log = (message, level = 'error') => emit(process.stderr, { event: 'log', level, message });

/*
 * progress events on stdout with --progress
 *
 * { "event": "progress", "command": "cutout", "stage": "model",
 *   "message": "running u2netp at 320x320", "percent": 23, "elapsedMs": 412 }
 *
 * stage weights come from PLANS, percent never goes back. The last line
 * of stdout is always the result event.
 */
const progress = {
  on: false,
  command: '',
  plan: [],
  total: 1,
  base: 0,
  weight: 0,
  percent: 0,
  stage_: '',
  message: '',
  shown: -1,
  t0: now(),

  start(command, on, plan) {
    Object.assign(this, { on, command, plan, total: plan.reduce((n, [, w]) => n + w, 0) || 1, t0: now(), percent: 0, shown: -1 });
  },

  report() {
    const percent = Math.round(this.percent);

    if(!this.on || percent == this.shown && this.message == this.last) return;

    this.shown = percent;
    this.last = this.message;
    emit(process.stdout, { event: 'progress', command: this.command, stage: this.stage_, message: this.message, percent, elapsedMs: ms(this.t0) });
  },

  // a new stage starts, its weight decides where percent begins
  stage(name, message) {
    let before = 0;

    this.stage_ = name;
    this.message = message;
    this.base = this.percent;
    this.weight = 0;

    for(const [n, w] of this.plan) {
      if(n == name) {
        this.base = (before / this.total) * 100;
        this.weight = (w / this.total) * 100;
        break;
      }

      before += w;
    }

    this.percent = Math.max(this.percent, this.base);
    this.report();
  },

  // fraction 0..1 of the current stage is done
  sub(fraction, detail) {
    this.percent = Math.max(this.percent, this.base + this.weight * Math.min(1, fraction));
    this.message = this.message.replace(/ \(.*\)$/, '') + (detail ? ' (' + detail + ')' : '');
    this.report();
  },

  finish() {
    this.stage_ = 'done';
    this.message = 'finished';
    this.percent = 100;
    this.report();
  },
};

const PLANS = {
  cutout: o => [['read', 4], ['load', 3], ['model', 12], ['refine', o.refine == 'matting' ? 30 : 12], ['subjects', 8], ...(o.edgeKey == 'none' ? [] : [['edge', 12]]), ['colour', 17], ['write', 10], ...(o.restOut ? [['rest', 14]] : []), ...(o.preview ? [['preview', 2]] : [])],
  palette: () => [['read', 50], ['palette', 50]],
  info: () => [['read', 100]],
  clip: () => [['read', 30], ['clip', 50], ['write', 20]],
  capabilities: () => [['probe', 100]],
  prepare: () => [['read', 8], ['encode', 92]],
  segment: o => [['read', 4], ['encode', 40], ['decode', 4], ['refine', 8], ['subjects', 4], ...(o.edgeKey == 'none' ? [] : [['edge', 8]]), ['colour', 12], ['write', 8], ...(o.restOut ? [['rest', 10]] : []), ...(o.preview ? [['preview', 2]] : [])],
};

const USAGE = `opencv-imgproc.js <command> [options]

  capabilities [--no-probe]
  info    --in FILE
  palette --in FILE [--count 12] [--dist cube|cie76|cie94|kmeans] [--hsv]
  clip    --in FILE --out FILE --polygon JSON [--invert] [--supersample 4]
  prepare --in FILE [--model mobilesam|sam] [--cache-dir DIR]
  segment --in FILE --out FILE --point x,y[,label] [--point ...] [--box x,y,w,h]
          [--units px|norm] [--candidates 1..3] [--pick best|0|1|2]
          [--refine none|guided|matting] [--rest-out FILE] [--trim] ...
  cutout  --in FILE --out FILE [--progress] [--method dnn] [--dnn-model u2netp]
          [--refine none|guided|matting] [--subjects largest|all]
          [--split-dir DIR] [--min-area 0.01]
          [--rest-out FILE] [--rest-fill none|inpaint|fsr]
          [--edge-key auto|none|#rrggbb] [--edge-strength 0..1] [--edge-band N]
          [--trim] [--trim-pad N] [--outline-out FILE] [--outline-epsilon 1.5]
          [--mask-out FILE] [--preview FILE] [--models-dir DIR]

options are --name value or --name=value, --job FILE.json gives the same
keys as one object, flags override it. every command takes --progress.
all output is NDJSON, the last stdout line is the result event.
spec: doc/opencv-imgproc.md`;

/* option parsing, flag names are the job keys in kebab-case */

const BOOLS = new Set(['trim', 'invert', 'hsv', 'probe', 'help', 'progress']);
const LISTS = new Set(['point', 'fg', 'bg']);
const NUMS = new Set(['previewSize', 'candidates', 'count', 'supersample', 'minArea', 'trimPad', 'outlineEpsilon', 'edgeBand', 'edgeStrength', 'maxPixels', 'workSize', 'seed', 'png']);

const camel = k => k.replace(/-([a-z])/g, (_, c) => c.toUpperCase());

function parseArgs(argv) {
  const opts = {};
  const rest = [];

  for(let i = 0; i < argv.length; i++) {
    const arg = argv[i];

    if(!arg.startsWith('--')) {
      rest.push(arg);
      continue;
    }

    let name = arg.slice(2);
    let value;
    const eq = name.indexOf('=');

    if(eq >= 0) {
      value = name.slice(eq + 1);
      name = name.slice(0, eq);
    }

    let key = camel(name);

    if(/^no[A-Z]/.test(key) && BOOLS.has(key[2].toLowerCase() + key.slice(3))) {
      key = key[2].toLowerCase() + key.slice(3);
      value = 'false';
    }

    if(BOOLS.has(key)) value = value === undefined ? true : value !== 'false';
    else if(value === undefined) value = argv[++i];

    if(value === undefined) fail('E_USAGE', 'missing value for --' + name);

    if(LISTS.has(key)) (opts[key] ??= []).push(value);
    else opts[key] = NUMS.has(key) ? Number(value) : value;
  }

  return { cmd: rest[0], opts };
}

function loadJob(opts) {
  if(!opts.job) return opts;

  let job;

  try {
    job = JSON.parse(fs.readFileSync(opts.job, 'utf8'));
  } catch(e) {
    fail('E_USAGE', 'cannot read job file ' + opts.job + ': ' + e.message);
  }

  if(job === null || typeof job != 'object' || Array.isArray(job)) fail('E_USAGE', 'job file must hold one object');

  return { ...job, ...opts };
}

/* models, never bundled: files come from the models directory */

const MODELS = {
  u2netp: {
    kind: 'salient',
    files: { main: 'u2netp.onnx' },
    sha1: { main: '0a99236f0d5c1916a99a8c401b23e5ef32038606' },
    inputSize: [320, 320],
    mean: [123.6, 116.2, 103.5],
    scale: 0.019,
    licence: 'Apache-2.0',
    commercial: true,
  },
  sam: {
    kind: 'promptable',
    files: { main: 'vision_encoder.onnx', decoder: 'prompt_encoder_mask_decoder.onnx' },
    sha1: { main: '305e9bac1b6ca9358bebb5d24afcc63a43100a7f', decoder: '81184f05faf4a6b85e63e4fee09f438dc614004d' },
    iface: 'hf',
    inputSize: [1024, 1024],
    licence: 'Apache-2.0',
    commercial: true,
  },
  // Acly/MobileSAM onnx export: original-SAM decoder interface, the image
  // normalisation sits inside the encoder graph, no positional embedding
  mobilesam: {
    kind: 'promptable',
    files: { main: 'mobile_sam_image_encoder.onnx', decoder: 'sam_mask_decoder_multi.onnx' },
    sha1: { main: '47a62fc254513b615e104257fbf4c316114b35c2', decoder: 'e77a4c907e52ac2863f05a3b84e6727a5c7a5449' },
    iface: 'orig',
    inputSize: [1024, 1024],
    licence: 'Apache-2.0 (weights), MIT (onnx export)',
    commercial: true,
  },
};

const scriptDir = import.meta.url.replace(/^file:\/\//, '').replace(/[^/]*$/, '');

function modelsDir(opts) {
  return opts.modelsDir ?? process.env.WB_MODELS_DIR ?? path.join(scriptDir, '..', 'models');
}

function loadNet(opts, name, part = 'main') {
  const spec = MODELS[name] ?? fail('E_MODEL', 'unknown model ' + name, { known: Object.keys(MODELS) });
  const file = path.join(modelsDir(opts), spec.files[part]);

  if(!fs.existsSync(file)) fail('E_MODEL', 'model file missing: ' + file);

  try {
    return cv.readNetFromONNX(file, cv.dnn.ENGINE_AUTO);
  } catch(e) {
    fail('E_MODEL', 'model does not load: ' + file, { opencv: String(e.message).split('\n')[0] });
  }
}

/* reading images */

const toU8 = buf => (buf instanceof Uint8Array ? buf : new Uint8Array(buf));

// exif orientation 1..8 of a jpeg, null when there is none
function jpegOrientation(u8) {
  if(u8[0] != 0xff || u8[1] != 0xd8) return null;

  for(let i = 2; i + 4 < u8.length; ) {
    if(u8[i] != 0xff) return null;

    const marker = u8[i + 1];
    const len = (u8[i + 2] << 8) | u8[i + 3];

    if(marker == 0xe1 && u8[i + 4] == 0x45 && u8[i + 5] == 0x78) {
      const t = i + 10;
      const le = u8[t] == 0x49;
      const u16 = o => (le ? u8[o] | (u8[o + 1] << 8) : (u8[o] << 8) | u8[o + 1]);
      const u32 = o => (le ? u16(o) | (u16(o + 2) << 16) : (u16(o) << 16) | u16(o + 2));
      const ifd = t + u32(t + 4);

      for(let n = u16(ifd), e = ifd + 2; n > 0; n--, e += 12) if(u16(e) == 0x0112) return u16(e + 8);

      return null;
    }

    if(marker == 0xda) return null;

    i += 2 + len;
  }

  return null;
}

// width and height from the header of a png or jpeg, null if unknown
function headerSize(u8) {
  if(u8[0] == 0x89 && u8[1] == 0x50) {
    const u32 = o => ((u8[o] << 24) | (u8[o + 1] << 16) | (u8[o + 2] << 8) | u8[o + 3]) >>> 0;

    return [u32(16), u32(20)];
  }

  if(u8[0] == 0xff && u8[1] == 0xd8)
    for(let i = 2; i + 9 < u8.length; ) {
      if(u8[i] != 0xff) return null;

      const marker = u8[i + 1];

      if(marker >= 0xc0 && marker <= 0xcf && marker != 0xc4 && marker != 0xc8 && marker != 0xcc) return [(u8[i + 7] << 8) | u8[i + 8], (u8[i + 5] << 8) | u8[i + 6]];

      i += 2 + ((u8[i + 2] << 8) | u8[i + 3]);
    }

  return null;
}

// bgr is 8-bit 3 channel with the exif orientation applied
function readImage(file, maxPixels = 64000000) {
  if(!file) fail('E_USAGE', 'missing --in');

  progress.stage('read', 'reading and decoding ' + path.basename(file));

  let u8;

  try {
    u8 = toU8(fs.readFileSync(file));
  } catch(e) {
    fail('E_INPUT', 'cannot read ' + file);
  }

  if(u8.length == 0) fail('E_INPUT', 'empty file ' + file);

  const size = headerSize(u8);

  if(size && size[0] * size[1] > maxPixels) fail('E_TOO_LARGE', 'image has ' + size[0] * size[1] + ' pixels', { maxPixels });

  if(u8[0] == 0x00 && u8[4] == 0x66 && u8[8] == 0x68) fail('E_FORMAT', 'HEIC input, convert to jpg or png first');

  let bgr;

  try {
    bgr = cv.imdecode(u8, cv.IMREAD_COLOR);
  } catch(e) {}

  if(!bgr || bgr.empty()) fail('E_INPUT', 'cannot decode ' + file);

  if(bgr.cols * bgr.rows > maxPixels) fail('E_TOO_LARGE', 'image has ' + bgr.cols * bgr.rows + ' pixels', { maxPixels });

  let alpha = null;

  if(u8[0] == 0x89 || u8[0] == 0x52 || u8[0] == 0x49 || u8[0] == 0x4d) {
    const full = cv.imdecode(u8, cv.IMREAD_UNCHANGED);

    if(full && full.channels() == 4) alpha = cv.split(full)[3];
  }

  return { u8, bgr, alpha, width: bgr.cols, height: bgr.rows, channels: alpha ? 4 : 3, hasAlpha: !!alpha, orientation: jpegOrientation(u8) };
}

/* float helpers, all images CV_32F in 0..1 */

const toFloat = m => {
  const f = new cv.Mat();
  m.convertTo(f, cv.CV_32F, 1 / 255);
  return f;
};

const toU8Mat = f => {
  const o = new cv.Mat();
  f.convertTo(o, cv.CV_8U, 255);
  return o;
};

const clamp01 = m => {
  const hi = new cv.Mat();
  const lo = new cv.Mat();

  cv.threshold(m, hi, 1, 1, cv.THRESH_TRUNC);
  cv.threshold(hi, lo, 0, 0, cv.THRESH_TOZERO);

  return lo;
};

// steepen around 0.5, k = 1 leaves the matte alone
function contrast(m, k) {
  const o = new cv.Mat();

  m.convertTo(o, cv.CV_32F, k, 0.5 - 0.5 * k);

  return clamp01(o);
}

const to3 = a => {
  const o = new cv.Mat();
  cv.merge([a, a, a], o);
  return o;
};

const oneMinus = a => {
  const o = new cv.Mat();
  a.convertTo(o, cv.CV_32F, -1, 1);
  return o;
};

const mul = (a, b) => {
  const o = new cv.Mat();
  cv.multiply(a, b, o);
  return o;
};

const add = (a, b) => {
  const o = new cv.Mat();
  cv.add(a, b, o);
  return o;
};

const box = (m, r) => {
  const o = new cv.Mat();
  cv.blur(m, o, new cv.Size(2 * r + 1, 2 * r + 1));
  return o;
};

const resizeTo = (m, w, h, how) => {
  const o = new cv.Mat();
  cv.resize(m, o, new cv.Size(w, h), 0, 0, how);
  return o;
};

// longest edge at most size, never enlarges
function workCopy(m, size) {
  const k = Math.min(1, size / Math.max(m.cols, m.rows));

  return k < 1 ? resizeTo(m, Math.round(m.cols * k), Math.round(m.rows * k), cv.INTER_AREA) : m;
}

const STRIP = 512;

const rowsOf = (m, y0, y1) => m.roi(new cv.Rect(0, y0, m.cols, y1 - y0));

// run fn on horizontal strips that overlap by margin rows and paste each
// core into one output, so large images never need full-size temporaries.
// fn gets the row range y0..y1 of the strip and returns a Mat of those rows
function stripwise(cols, rows, type, margin, fn) {
  const out = new cv.Mat(rows, cols, type);

  for(let y = 0; y < rows; y += STRIP) {
    const n = Math.min(STRIP, rows - y);

    progress.sub(y / rows, 'strip ' + (y / STRIP + 1) + '/' + Math.ceil(rows / STRIP));
    const y0 = Math.max(0, y - margin);

    fn(y0, Math.min(rows, y + n + margin))
      .roi(new cv.Rect(0, y - y0, cols, n))
      .copyTo(out.roi(new cv.Rect(0, y, cols, n)));
  }

  return out;
}

// colour layer over a flat colour, color is [b, g, r] in 0..1
function compositeFlat(bgrF, alphaF, color) {
  const a3 = to3(alphaF);
  const bg = new cv.Mat(bgrF.rows, bgrF.cols, cv.CV_32FC3, new cv.Scalar(...color));

  return add(mul(bgrF, a3), mul(bg, oneMinus(a3)));
}

function checker(w, h, cell) {
  const m = new cv.Mat(h, w, cv.CV_32FC3, new cv.Scalar(0.8, 0.8, 0.8));

  for(let y = 0; y < h; y += cell)
    for(let x = (y / cell) % 2 ? 0 : cell; x < w; x += 2 * cell) cv.rectangle(m, new cv.Point(x, y), new cv.Point(x + cell - 1, y + cell - 1), new cv.Scalar(0.55, 0.55, 0.55), -1);

  return m;
}

// checkerboard, green and magenta side by side, for judging edges
function writePreview(file, bgr, alphaF) {
  const w = 480;
  const h = Math.round((bgr.rows * w) / bgr.cols);
  const b = toFloat(resizeTo(bgr, w, h, cv.INTER_AREA));
  const a = resizeTo(alphaF, w, h, cv.INTER_AREA);
  const checkerOver = add(mul(b, to3(a)), mul(checker(w, h, 16), oneMinus(to3(a))));
  const tiles = [checkerOver, compositeFlat(b, a, [0, 1, 0]), compositeFlat(b, a, [1, 0, 1])];
  const out = new cv.Mat();

  cv.hconcat(tiles, out);
  writeFile(file, cv.imencode('.png', toU8Mat(out)));
}

/* output */

function writeFile(file, bytes) {
  const tmp = file + '.tmp-' + process.pid;

  try {
    fs.writeFileSync(tmp, typeof bytes == 'string' ? bytes : new Uint8Array(bytes));
    fs.renameSync(tmp, file);
  } catch(e) {
    try {
      fs.unlinkSync(tmp);
    } catch(_) {}

    fail('E_WRITE', 'cannot write ' + file + ': ' + e.message);
  }
}

function encode(file, mat, opts) {
  const ext = path.extname(file).toLowerCase();

  if(ext == '.webp') return cv.imencode(ext, mat, { quality: opts.webp === undefined || opts.webp == 'lossless' ? 101 : Number(opts.webp) });
  if(ext == '.png' && opts.png !== undefined) return cv.imencode(ext, mat, { compression: Number(opts.png) });

  return cv.imencode(ext, mat);
}

function checkOutPath(file, what = '--out') {
  if(!file) fail('E_USAGE', 'missing ' + what);
  if(!/\.(png|webp)$/i.test(file)) fail('E_FORMAT', what + ' must be .png or .webp, the output carries alpha', { out: file });
}

const bboxOf = a8 => {
  const m = new cv.Mat();
  cv.threshold(a8, m, 0, 255, cv.THRESH_BINARY);

  const r = cv.boundingRect(m);

  return { x: r.x, y: r.y, w: r.width, h: r.height };
};

// 8-bit colour and a float alpha in 0..1 as one BGRA image
function bgra(bgr8, alphaF) {
  const out = new cv.Mat();

  cv.merge([...cv.split(bgr8), toU8Mat(alphaF)], out);

  return out;
}

function outline(alpha8, epsilon) {
  const bin = new cv.Mat();
  const cs = new cv.MatVector();

  cv.threshold(alpha8, bin, 127, 255, cv.THRESH_BINARY);
  cv.findContours(bin, cs, new cv.Mat(), cv.RETR_EXTERNAL, cv.CHAIN_APPROX_NONE);

  let best = null;

  for(let i = 0; i < cs.size(); i++) if(!best || cv.contourArea(cs.get(i)) > cv.contourArea(best)) best = cs.get(i);

  if(!best) return [];

  const approx = new cv.Mat();
  cv.approxPolyDP(best, approx, epsilon, true);

  const d = approx.data32S;
  const poly = [];

  for(let i = 0; i < d.length; i += 2) poly.push([d[i], d[i + 1]]);

  return poly;
}

/* the matte: model, refinement, subjects, colours */

// probability map of a salient-object model, 0..1 at the model size
function runSalient(net, spec, bgr) {
  const [mw, mh] = spec.inputSize;
  const work = workCopy(bgr, 1280);

  net.setInput(cv.blobFromImage(work, spec.scale, new cv.Size(mw, mh), new cv.Scalar(...spec.mean), true, false));

  const out = net.forward(net.getUnconnectedOutLayersNames())[0];
  const prob = new cv.Mat(mh, mw, cv.CV_32F, out.data32F.slice().buffer);
  const { minVal, maxVal } = cv.minMaxLoc(prob);
  const norm = new cv.Mat();
  const range = Math.max(maxVal - minVal, 1e-6);

  prob.convertTo(norm, cv.CV_32F, 1 / range, -minVal / range);

  return norm;
}

// guided filter radius for an image, px
const guideRadius = long => Math.max(4, Math.round(long * 0.006));

// snap a soft map to the photo's edges: small guided filter, then steepen
function refineGuided(bgr, soft) {
  const r = guideRadius(Math.max(bgr.cols, bgr.rows));
  const g = stripwise(bgr.cols, bgr.rows, cv.CV_32F, 3 * r, (y0, y1) => {
    const out = new cv.Mat();

    cv.ximgproc.guidedFilter(toFloat(rowsOf(bgr, y0, y1)), rowsOf(soft, y0, y1), out, r, 1e-3, -1);

    return out;
  });

  return contrast(g, 4);
}

// trimap from a soft matte, 255 sure foreground, 0 sure background
function trimapOf(soft, band) {
  const bin = new cv.Mat();
  const fg = new cv.Mat();
  const bgNear = new cv.Mat();
  const kernel = cv.getStructuringElement(cv.MORPH_ELLIPSE, new cv.Size(2 * band + 1, 2 * band + 1));

  cv.threshold(toU8Mat(soft), bin, 127, 255, cv.THRESH_BINARY);
  cv.erode(bin, fg, kernel);
  cv.dilate(bin, bgNear, kernel);

  const tri = new cv.Mat(soft.rows, soft.cols, cv.CV_8UC1, new cv.Scalar(128));

  tri.setTo(new cv.Scalar(255), fg);

  const bgMask = new cv.Mat();
  cv.bitwise_not(bgNear, bgMask);
  tri.setTo(new cv.Scalar(0), bgMask);

  return tri;
}

// information-flow matting in the unknown band at work size, guided up
function refineMatting(bgr, soft, workSize) {
  const work = workCopy(bgr, workSize);
  const small = resizeTo(soft, work.cols, work.rows, cv.INTER_AREA);
  const tri = trimapOf(small, Math.max(3, Math.round(Math.max(work.cols, work.rows) * 0.015)));
  const alphaSmall = new cv.Mat();

  cv.alphamat.infoFlow(work, tri, alphaSmall);

  return refineGuided(bgr, resizeTo(toFloat(alphaSmall), bgr.cols, bgr.rows, cv.INTER_CUBIC));
}

// connected parts of a soft matte, biggest first, at work size
function findSubjects(soft, minArea, subjects, seeds) {
  const small = workCopy(soft, 1024);
  const bin = new cv.Mat();
  const labels = new cv.Mat();
  const stats = new cv.Mat();

  cv.threshold(toU8Mat(small), bin, 127, 255, cv.THRESH_BINARY);

  const n = cv.connectedComponentsWithStats(bin, labels, stats, new cv.Mat(), 8, cv.CV_32S);
  const total = small.cols * small.rows;
  const parts = [];

  for(let i = 1; i < n; i++) {
    const area = stats.data32S[i * 5 + 4];

    if(area / total >= minArea) parts.push({ label: i, area, fraction: area / total });
  }

  parts.sort((a, b) => b.area - a.area);

  // parts under a tap come first, else the biggest, or all of them
  const k = small.cols / soft.cols;
  const under = seeds?.length ? parts.filter(p => seeds.some(([x, y]) => labels.data32S[Math.min(small.rows - 1, Math.round(y * k)) * small.cols + Math.min(small.cols - 1, Math.round(x * k))] == p.label)) : [];
  const chosen = under.length ? under : subjects == 'all' ? parts : parts.slice(0, 1);

  return { labels, small, chosen, found: n - 1 };
}

// 0..1 float mask at full size that lets one part's soft edge through
function partGate(sub, part, w, h, grow) {
  const mask = new cv.Mat();
  const filled = new cv.Mat();
  const k = sub.small.cols / w;
  const kernel = cv.getStructuringElement(cv.MORPH_RECT, new cv.Size(2 * Math.max(1, Math.round(grow * k)) + 1, 2 * Math.max(1, Math.round(grow * k)) + 1));

  cv.compare(sub.labels, new cv.Scalar(part.label), mask, cv.CMP_EQ);

  // holes smaller than 0.5% of the part are filled, bigger ones are real
  const inv = new cv.Mat();
  const hl = new cv.Mat();
  const hs = new cv.Mat();

  cv.bitwise_not(mask, inv);

  const hn = cv.connectedComponentsWithStats(inv, hl, hs, new cv.Mat(), 4, cv.CV_32S);

  mask.copyTo(filled);

  for(let i = 1; i < hn; i++) {
    const x = hs.data32S[i * 5];
    const y = hs.data32S[i * 5 + 1];
    const bw = hs.data32S[i * 5 + 2];
    const bh = hs.data32S[i * 5 + 3];
    const touches = x == 0 || y == 0 || x + bw == mask.cols || y + bh == mask.rows;

    if(!touches && hs.data32S[i * 5 + 4] < part.area * 0.005) {
      const hole = new cv.Mat();

      cv.compare(hl, new cv.Scalar(i), hole, cv.CMP_EQ);
      filled.setTo(new cv.Scalar(255), hole);
    }
  }

  const grown = new cv.Mat();

  cv.dilate(filled, grown, kernel);

  return { gate: resizeTo(toFloat(grown), w, h, cv.INTER_LINEAR), solid: resizeTo(toFloat(filled), w, h, cv.INTER_LINEAR) };
}

const sumChannels = m => {
  const [a, b, c] = cv.split(m);

  return add(add(a, b), c);
};

const hexColor = h => {
  const m = /^#?([0-9a-f]{2})([0-9a-f]{2})([0-9a-f]{2})$/i.exec(h);

  return m ? [m[3], m[2], m[1]].map(v => parseInt(v, 16) / 255) : null;
};

// background colour next to the cut, from pixels clear of the matte
function localBackground(bgrF, alphaF, band, long) {
  const near = new cv.Mat();
  const clear = new cv.Mat();
  const a8 = toU8Mat(alphaF);

  cv.threshold(a8, near, 2, 255, cv.THRESH_BINARY);
  cv.dilate(near, near, cv.getStructuringElement(cv.MORPH_RECT, new cv.Size(2 * band + 1, 2 * band + 1)));
  cv.bitwise_not(near, clear);

  const w = toFloat(clear);
  const r = Math.max(15, Math.round(long * 0.0225));
  const den = new cv.Mat();

  cv.add(box(w, r), new cv.Scalar(1e-5), den);

  const out = new cv.Mat();

  cv.divide(box(mul(bgrF, to3(w)), r), to3(den), out);

  return out;
}

// colour to alpha at the cut edge only: a pixel in the edge band that
// looks like the background (or the key colour) loses its alpha
function defringeCore(bgrF, alphaF, key, band, strength, long) {
  const bin = new cv.Mat();
  const inner = new cv.Mat();
  const outer = new cv.Mat();
  const kernel = cv.getStructuringElement(cv.MORPH_ELLIPSE, new cv.Size(2 * band + 1, 2 * band + 1));

  cv.threshold(toU8Mat(alphaF), bin, 127, 255, cv.THRESH_BINARY);
  cv.erode(bin, inner, kernel);
  cv.dilate(bin, outer, kernel);

  // edge band weight, 1 on the edge, 0 inside and far outside
  const ring = new cv.Mat();
  const ringF = new cv.Mat();

  cv.subtract(outer, inner, ring);
  cv.GaussianBlur(toFloat(ring), ringF, new cv.Size(0, 0), Math.max(1, band / 3));

  const bg = key ? new cv.Mat(bgrF.rows, bgrF.cols, cv.CV_32FC3, new cv.Scalar(...key)) : localBackground(bgrF, alphaF, band, long);

  // interior colour next to the edge
  const r = Math.max(8, band * 4);
  const iw = toFloat(inner);
  const den = new cv.Mat();
  const fg = new cv.Mat();

  cv.add(box(iw, r), new cv.Scalar(1e-5), den);
  cv.divide(box(mul(bgrF, to3(iw)), r), to3(den), fg);

  // how far along the way from background to foreground colour
  const g = new cv.Mat();
  const d = new cv.Mat();

  cv.subtract(fg, bg, g);
  cv.subtract(bgrF, bg, d);

  const gg = sumChannels(mul(g, g));
  const proj = new cv.Mat();
  const gd = new cv.Mat();

  cv.add(gg, new cv.Scalar(1e-4), gd);
  cv.divide(sumChannels(mul(d, g)), gd, proj);

  // ramp 0.2 .. 0.8, and no change where fg and bg colours are alike
  const ramp = new cv.Mat();

  proj.convertTo(ramp, cv.CV_32F, 1 / 0.6, -0.2 / 0.6);

  const similar = new cv.Mat();

  gg.convertTo(similar, cv.CV_32F, 1 / 0.1, -0.3);

  const sure = clamp01(similar);
  const w = mul(mul(ringF, sure), new cv.Mat(bgrF.rows, bgrF.cols, cv.CV_32F, new cv.Scalar(strength)));
  const target = new cv.Mat();

  cv.min(alphaF, clamp01(ramp), target);

  // alpha = alpha + w * (target - alpha)
  const diff = new cv.Mat();

  cv.subtract(target, alphaF, diff);

  // a median on the edge ring only removes isolated leftover specks
  const cleaned = add(alphaF, mul(w, diff));
  const med = new cv.Mat();

  cv.medianBlur(cleaned, med, 3);

  cv.subtract(med, cleaned, diff);

  return add(cleaned, mul(ringF, diff));
}

// first and last row that has any alpha, a strip beyond them is skipped
function liveRows(alphaF) {
  const mask = new cv.Mat();

  cv.threshold(toU8Mat(alphaF), mask, 0, 255, cv.THRESH_BINARY);

  const r = cv.boundingRect(mask);

  return { y0: r.y, y1: r.y + r.height };
}

// colour to alpha at the cut edge, strip-wise on the 8-bit photo
function defringe(bgr, alphaF, key, band, strength) {
  const long = Math.max(bgr.cols, bgr.rows);
  const margin = Math.max(15, Math.round(long * 0.0225)) + 8 * band + 4;
  const live = liveRows(alphaF);

  return stripwise(bgr.cols, bgr.rows, cv.CV_32F, margin, (y0, y1) => (y1 <= live.y0 || y0 >= live.y1 ? rowsOf(alphaF, y0, y1) : defringeCore(toFloat(rowsOf(bgr, y0, y1)), rowsOf(alphaF, y0, y1), key, band, strength, long)));
}

// blur fusion (Germer 2020) on one strip, radii come from the full image
function fusionCore(bgrF, alphaF, r1, r2) {
  const eps = new cv.Scalar(1e-5);

  const pass = (F, B, r) => {
    const a3 = to3(alphaF);
    const ia3 = oneMinus(a3);
    const ba = box(alphaF, r);
    const bFa = box(mul(F, a3), r);
    const bBa = box(mul(B, ia3), r);
    const bF = new cv.Mat();
    const bB = new cv.Mat();
    const d1 = new cv.Mat();
    const d2 = new cv.Mat();

    cv.add(ba, eps, d1);
    cv.add(oneMinus(ba), eps, d2);
    cv.divide(bFa, to3(d1), bF);
    cv.divide(bBa, to3(d2), bB);

    const resid = new cv.Mat();

    cv.subtract(bgrF, add(mul(a3, bF), mul(ia3, bB)), resid);

    return [clamp01(add(bF, mul(a3, resid))), bB];
  };

  const [F1, B1] = pass(bgrF, bgrF, r1);

  return pass(F1, B1, r2)[0];
}

// foreground colour without background bleed, an 8-bit BGR image
function estimateForeground(bgr, alphaF) {
  const long = Math.max(bgr.cols, bgr.rows);
  const r1 = Math.max(15, Math.round(long * 0.0225));
  const r2 = Math.max(2, Math.round(long * 0.0015));

  const live = liveRows(alphaF);

  return stripwise(bgr.cols, bgr.rows, cv.CV_8UC3, r1 + r2 + 2, (y0, y1) => (y1 <= live.y0 || y0 >= live.y1 ? rowsOf(bgr, y0, y1) : toU8Mat(fusionCore(toFloat(rowsOf(bgr, y0, y1)), rowsOf(alphaF, y0, y1), r1, r2))));
}

// fill the hole a cut leaves, at work size, blended back at full size.
// telea is fast and smooth, fsr is xphoto frequency selective, much slower
function fillHole(bgr, alphaF, how) {
  const long = Math.max(bgr.cols, bgr.rows);
  const work = workCopy(bgr, how == 'fsr' ? 512 : 1024);
  const k = work.cols / bgr.cols;
  const r = Math.max(2, Math.round(long * 0.004));
  const rw = Math.max(1, Math.round(r * k));
  const hole = new cv.Mat();
  const filled = new cv.Mat();

  cv.threshold(toU8Mat(resizeTo(alphaF, work.cols, work.rows, cv.INTER_AREA)), hole, 4, 255, cv.THRESH_BINARY);
  cv.dilate(hole, hole, cv.getStructuringElement(cv.MORPH_ELLIPSE, new cv.Size(2 * rw + 1, 2 * rw + 1)));

  if(how == 'fsr') {
    const keep = new cv.Mat();

    cv.bitwise_not(hole, keep);
    cv.xphoto.inpaint(work, keep, filled, cv.xphoto.INPAINT_FSR_FAST);
  } else cv.inpaint(work, hole, filled, 3, cv.INPAINT_TELEA);

  const up = resizeTo(filled, bgr.cols, bgr.rows, cv.INTER_CUBIC);
  const holeF = new cv.Mat();

  cv.GaussianBlur(resizeTo(toFloat(hole), bgr.cols, bgr.rows, cv.INTER_LINEAR), holeF, new cv.Size(0, 0), Math.max(1, r / 2));

  return stripwise(bgr.cols, bgr.rows, cv.CV_8UC3, 0, (y0, y1) => {
    const h3 = to3(rowsOf(holeF, y0, y1));

    return toU8Mat(add(mul(toFloat(rowsOf(bgr, y0, y1)), oneMinus(h3)), mul(toFloat(rowsOf(up, y0, y1)), h3)));
  });
}

/* promptable segmentation, SAM: the encoder runs once per photo and its
 * output is cached, the decoder runs per tap */

const cacheDir = opts => opts.cacheDir ?? process.env.WB_CACHE_DIR ?? path.join(scriptDir, '..', 'cache');

// longest edge scaled to T, normalised per channel, zero padded to a square
function samPreprocess(bgr, T, iface = 'hf') {
  const scale = T / Math.max(bgr.cols, bgr.rows);
  const newW = Math.round(bgr.cols * scale);
  const newH = Math.round(bgr.rows * scale);
  const rgb = new cv.Mat();

  // the mobilesam encoder normalises and pads itself: raw 0..255 rgb, HWC
  if(iface == 'orig') {
    cv.cvtColor(bgr, rgb, cv.COLOR_BGR2RGB);

    const f = new cv.Mat();

    resizeTo(rgb, newW, newH, cv.INTER_LINEAR).convertTo(f, cv.CV_32F);

    return { blob: ownMat(f.data32F, 1, newW * newH * 3, [newH, newW, 3]), newW, newH, scale };
  }

  const mean = [0.485, 0.456, 0.406];
  const std = [0.229, 0.224, 0.225];
  const buf = new Float32Array(3 * T * T);

  cv.cvtColor(bgr, rgb, cv.COLOR_BGR2RGB);

  cv.split(toFloat(resizeTo(rgb, newW, newH, cv.INTER_LINEAR))).forEach((plane, i) => {
    const norm = new cv.Mat();
    const padded = new cv.Mat();

    plane.convertTo(norm, cv.CV_32F, 1 / std[i], -mean[i] / std[i]);
    cv.copyMakeBorder(norm, padded, 0, T - newH, 0, T - newW, cv.BORDER_CONSTANT, new cv.Scalar(0));
    buf.set(padded.data32F, i * T * T);
  });

  return { blob: ownMat(buf, 1, 3 * T * T, [1, 3, T, T]), newW, newH, scale };
}

const EMB = [1, 256, 64, 64];
const EMB_N = 256 * 64 * 64;

// a float Mat that owns a copy of src. new Mat(.., typedArray.buffer) only
// borrows, and its reshape does not keep the array alive
function ownMat(src, rows, cols, dims) {
  const m = new cv.Mat(rows, cols, cv.CV_32F);

  m.data32F.set(src);

  return dims ? m.reshape(1, dims) : m;
}

const embMat = f32 => ownMat(f32, 1, EMB_N, EMB);

// encoder output for a photo, from the cache or computed and stored
function samEmbeddings(opts, img, name) {
  const spec = MODELS[name];
  const dir = cacheDir(opts);
  const key = cv.hash.sha256(img.u8) + '-' + name + '-' + spec.sha1.main.slice(0, 12);
  const file = path.join(dir, key + '.emb');
  const [T] = spec.inputSize;
  const t = now();

  if(fs.existsSync(file) && fs.existsSync(file + '.json')) {
    const raw = toU8(fs.readFileSync(file));
    const all = new Float32Array(raw.slice().buffer);
    const meta = JSON.parse(fs.readFileSync(file + '.json', 'utf8'));

    return { hit: true, key, file, meta, emb: embMat(all.slice(0, EMB_N)), pos: all.length > EMB_N ? embMat(all.slice(EMB_N)) : null, ms: ms(t) };
  }

  progress.stage('encode', 'running the ' + name + ' image encoder, a few seconds');

  const net = loadNet(opts, name, 'main');
  const pre = samPreprocess(img.bgr, T, spec.iface);
  let emb, pos = null;

  if(spec.iface == 'orig') {
    net.setInput(pre.blob, 'input_image');
    emb = net.forward('image_embeddings');
  } else {
    net.setInput(pre.blob, 'pixel_values');
    [emb, pos] = net.forward(['image_embeddings', 'image_positional_embeddings']);
  }

  const meta = { width: img.width, height: img.height, newW: pre.newW, newH: pre.newH, model: name, sha1: spec.sha1.main };
  const all = new Float32Array(pos ? 2 * EMB_N : EMB_N);

  all.set(emb.data32F, 0);
  if(pos) all.set(pos.data32F, EMB_N);

  try {
    fs.mkdirSync(dir, { recursive: true });
  } catch(e) {}

  writeFile(file, all.buffer);
  writeFile(file + '.json', JSON.stringify(meta));

  return { hit: false, key, file, meta, emb, pos, ms: ms(t) };
}

// taps and a rough box in photo px. the decoder takes points only, so a
// box becomes a tap at its centre and later picks the best fitting mask
function parsePrompts(opts, w, h) {
  const k = opts.units == 'norm' ? [w, h] : [1, 1];
  const points = [];
  let box = null;
  const bad = what => fail('E_USAGE', what);

  for(const text of opts.point ?? []) {
    const [x, y, label = 1] = String(text).split(',').map(Number);

    if(![x, y, label].every(Number.isFinite) || ![0, 1].includes(label)) bad('--point must be x,y or x,y,label with label 0 or 1: ' + text);

    points.push({ x: x * k[0], y: y * k[1], label });
  }

  if(opts.box) {
    const [x, y, bw, bh] = String(opts.box).split(',').map(Number);

    if(![x, y, bw, bh].every(Number.isFinite) || bw <= 0 || bh <= 0) bad('--box must be x,y,w,h: ' + opts.box);

    box = { x: x * k[0], y: y * k[1], w: bw * k[0], h: bh * k[1] };

    if(!points.some(p => p.label == 1)) points.push({ x: box.x + box.w / 2, y: box.y + box.h / 2, label: 1 });
  }

  if(!points.length) bad('segment needs --point x,y or --box x,y,w,h');

  for(const p of points) if(p.x < 0 || p.y < 0 || p.x >= w || p.y >= h) bad('prompt outside the photo (' + w + 'x' + h + '): ' + p.x + ',' + p.y);

  return { points, box };
}

// three candidate masks as 256x256 logits and their predicted iou
function samDecode(opts, name, emb, pos, points, scale) {
  const dec = loadNet(opts, name, 'decoder');
  const n = points.length;

  if(MODELS[name].iface == 'orig') return samDecodeOrig(dec, emb, points, scale);

  const coords = new Float32Array(points.flatMap(p => [p.x * scale, p.y * scale]));
  const labels = new BigInt64Array(points.map(p => BigInt(p.label)));

  dec.setInput(ownMat(coords, 1, n * 2, [1, 1, n, 2]), 'input_points');
  dec.setInput(new cv.Mat(1, n, cv.CV_64S, labels.buffer).reshape(1, [1, 1, n]), 'input_labels');
  dec.setInput(emb, 'image_embeddings');
  dec.setInput(pos, 'image_positional_embeddings');

  const [iou, masks] = dec.forward(['iou_scores', 'pred_masks']);
  const size = 256 * 256;
  const all = masks.data32F;

  return Array.from(iou.data32F).map((score, index) => ({ index, score, logits: ownMat(all.subarray(index * size, (index + 1) * size), 256, 256) }));
}

// original-SAM decoder: a padding point (label -1) closes the prompt list.
// it returns 4 masks, 0 is the multi-prompt one and 1..3 are sub-part, part
// and whole of a single tap, so one tap gets 1..3, several taps 0, 2, 3
function samDecodeOrig(dec, emb, points, scale) {
  const n = points.length + 1;
  const coords = new Float32Array([...points.flatMap(p => [p.x * scale, p.y * scale]), 0, 0]);
  const labels = new Float32Array([...points.map(p => p.label), -1]);

  dec.setInput(emb, 'image_embeddings');
  dec.setInput(ownMat(coords, 1, n * 2, [1, n, 2]), 'point_coords');
  dec.setInput(ownMat(labels, 1, n, [1, n]), 'point_labels');
  dec.setInput(ownMat(new Float32Array(256 * 256), 1, 256 * 256, [1, 1, 256, 256]), 'mask_input');
  dec.setInput(ownMat(new Float32Array(1), 1, 1, [1]), 'has_mask_input');
  // the graph rescales the taps from this size to the 1024 frame: 1024 keeps them as they are
  dec.setInput(ownMat(new Float32Array([1024, 1024]), 1, 2, [2]), 'orig_im_size');

  const [iou, low] = dec.forward(['iou_predictions', 'low_res_masks']);
  const size = 256 * 256;
  const all = low.data32F;

  return (points.length == 1 ? [1, 2, 3] : [0, 2, 3]).map((src, index) => ({ index, score: iou.data32F[src], logits: ownMat(all.subarray(src * size, (src + 1) * size), 256, 256) }));
}

// logits of the padded square to a soft matte at photo size
function samSoft(logits, T, pre, w, h) {
  const crop = resizeTo(logits, T, T, cv.INTER_LINEAR).roi(new cv.Rect(0, 0, pre.newW, pre.newH)).clone();
  const e = new cv.Mat();
  const prob = new cv.Mat();

  // sigmoid with slope 2, the guided filter sharpens the rest
  crop.convertTo(e, cv.CV_32F, -2);
  cv.exp(e, e);
  cv.add(e, new cv.Scalar(1), e);
  cv.divide(new cv.Mat(e.rows, e.cols, cv.CV_32F, new cv.Scalar(1)), e, prob);

  return resizeTo(prob, w, h, cv.INTER_CUBIC);
}

// candidate whose extent overlaps a box best, extents from the logits
function boxFit(found, box, long) {
  const k = long / 256;
  let top = { index: -1, iou: 0 };

  for(const f of found) {
    const bin = new cv.Mat();
    const r = new cv.Mat();

    cv.threshold(f.logits, bin, 0, 255, cv.THRESH_BINARY);
    bin.convertTo(r, cv.CV_8U);

    const b = cv.boundingRect(r);
    const [x0, y0, x1, y1] = [b.x * k, b.y * k, (b.x + b.width) * k, (b.y + b.height) * k];
    const iw = Math.max(0, Math.min(x1, box.x + box.w) - Math.max(x0, box.x));
    const ih = Math.max(0, Math.min(y1, box.y + box.h) - Math.max(y0, box.y));
    const inter = iw * ih;
    const iou = inter / ((x1 - x0) * (y1 - y0) + box.w * box.h - inter || 1);

    if(iou > top.iou) top = { index: f.index, iou };
  }

  return top.index < 0 ? null : top;
}

const samModel = opts => {
  const name = opts.model ?? 'mobilesam';

  if(MODELS[name]?.kind != 'promptable') fail('E_MODEL', name + ' is not a promptable model', { promptable: Object.keys(MODELS).filter(k => MODELS[k].kind == 'promptable') });

  return name;
};

function cmdPrepare(opts) {
  const t0 = now();
  const name = samModel(opts);
  const img = readImage(opts.in, opts.maxPixels);
  const e = samEmbeddings(opts, img, name);

  return { ok: true, command: 'prepare', cache: e.hit ? 'hit' : 'miss', key: e.key, path: e.file, bytes: fs.statSync(e.file).size, model: { name, licence: MODELS[name].licence }, input: { width: img.width, height: img.height }, timings: { prepareMs: e.ms, totalMs: ms(t0) } };
}

function cmdSegment(opts) {
  const t0 = now();
  const name = samModel(opts);
  const spec = MODELS[name];
  const refine = opts.refine ?? 'guided';
  const candidates = Math.max(1, Math.min(3, opts.candidates ?? 1));
  const timings = {};
  const warnings = [];

  checkOutPath(opts.out);

  if(opts.restOut) checkOutPath(opts.restOut, '--rest-out');
  if(!['none', 'guided', 'matting'].includes(refine)) fail('E_USAGE', '--refine must be none, guided or matting');
  if(opts.pick !== undefined && opts.pick != 'best' && !['0', '1', '2'].includes(String(opts.pick))) fail('E_USAGE', '--pick must be best, 0, 1 or 2');

  const full = readImage(opts.in, opts.maxPixels);
  const { points, box } = parsePrompts(opts, full.width, full.height);
  const e = samEmbeddings(opts, full, name);

  // --preview-size N: the whole cut runs on a photo reduced to N px, quick
  // taps for the editor, everything in the result is in those pixels
  const k = opts.previewSize && opts.previewSize < Math.max(full.width, full.height) ? opts.previewSize / Math.max(full.width, full.height) : 1;
  const img = k < 1 ? { ...full, bgr: workCopy(full.bgr, opts.previewSize), alpha: full.alpha && workCopy(full.alpha, opts.previewSize) } : full;

  img.width = img.bgr.cols;
  img.height = img.bgr.rows;

  const { bgr, width: w, height: h } = img;

  for(const p of points) Object.assign(p, { x: p.x * k, y: p.y * k });

  if(box) Object.assign(box, { x: box.x * k, y: box.y * k, w: box.w * k, h: box.h * k });

  timings.prepareMs = e.ms;

  let t = now();

  progress.stage('decode', 'segmenting at ' + points.length + (points.length == 1 ? ' prompt' : ' prompts'));

  const [T] = spec.inputSize;
  const pre = { newW: e.meta.newW, newH: e.meta.newH };
  const found = samDecode(opts, name, e.emb, e.pos, points, T / Math.max(full.width, full.height) / k);

  timings.decodeMs = ms(t);

  const best = found.reduce((a, b) => (b.score > a.score ? b : a)).index;
  const fit = box ? boxFit(found, box, Math.max(w, h)) : null;
  const picked = opts.pick === undefined || opts.pick == 'best' ? (fit?.index ?? best) : Number(opts.pick);
  const wanted = candidates > 1 ? found.slice(0, candidates).map(f => f.index) : [picked];
  const stem = opts.out.replace(/\.[^.]+$/, '');
  const ext = path.extname(opts.out);
  const taps = points.filter(p => p.label == 1).map(p => [p.x, p.y]);
  const list = [];
  let main;

  if(!wanted.includes(picked)) wanted.push(picked);

  for(const index of [picked, ...wanted.filter(i => i != picked)]) {
    const isMain = index == picked;
    const file = isMain ? opts.out : stem + '-' + index + ext;

    t = now();
    progress.stage('refine', 'refining candidate ' + index + ', ' + (refine == 'matting' ? 'alpha matting' : 'guided filter'));

    let soft = samSoft(found[index].logits, T, pre, w, h);

    if(refine == 'guided') soft = refineGuided(bgr, soft);
    else if(refine == 'matting') soft = refineMatting(bgr, soft, opts.workSize ?? 1024);
    else soft = clamp01(soft);

    const own = isMain ? opts : { ...opts, out: file, restOut: undefined, maskOut: undefined, outlineOut: undefined, preview: undefined };
    const result = cutFromSoft({ opts: own, img, soft, subjects: 'largest', minArea: opts.minArea ?? 0.0002, seeds: taps, timings: isMain ? timings : {}, warnings: isMain ? warnings : [], t0, command: 'segment', extra: {} });

    list.push({ index, score: Number(found[index].score.toFixed(4)), path: file, picked: isMain, coverage: result.mask.coverage, bbox: result.bbox });
    if(isMain) main = result;
  }

  // with several candidates every one has its own file, the picked one also is out
  if(candidates > 1) {
    const own = stem + '-' + picked + ext;

    fs.copyFileSync(opts.out, own);
    list.find(c => c.index == picked).path = own;
  }

  list.sort((a, b) => a.index - b.index);
  timings.totalMs = ms(t0);

  return { ...main, scale: k, original: { width: full.width, height: full.height }, cache: e.hit ? 'hit' : 'miss', model: { name, licence: spec.licence }, refine, candidates: list, picked, best, boxIou: fit ? Number(fit.iou.toFixed(4)) : undefined, prompts: points, timings };
}

/* commands */

function cmdCutout(opts) {
  const t0 = now();
  const timings = {};
  const warnings = [];
  const method = opts.method ?? 'auto';
  const modelName = opts.dnnModel ?? 'u2netp';
  const refine = opts.refine ?? 'guided';
  const subjects = opts.subjects ?? 'largest';
  const minArea = opts.minArea ?? 0.01;

  checkOutPath(opts.out);

  if(!['auto', 'dnn'].includes(method)) fail('E_METHOD', 'method ' + method + ' is not available, use dnn', { available: ['dnn'] });
  if(!['none', 'guided', 'matting'].includes(refine)) fail('E_USAGE', '--refine must be none, guided or matting');
  if(!['largest', 'all'].includes(subjects)) fail('E_USAGE', '--subjects must be largest or all');
  if(opts.restOut) checkOutPath(opts.restOut, '--rest-out');
  if(subjects == 'all' && !opts.splitDir) warnings.push('--subjects all without --split-dir: only the union is written');

  cv.setRNGSeed(opts.seed ?? 0);

  const spec = MODELS[modelName];

  if(spec?.kind != 'salient') fail('E_MODEL', modelName + ' is not a salient-object model', { salient: Object.keys(MODELS).filter(k => MODELS[k].kind == 'salient') });

  let t = now();
  const img = readImage(opts.in, opts.maxPixels);
  const { bgr, width: w, height: h } = img;
  const long = Math.max(w, h);

  timings.readMs = ms(t);
  t = now();

  progress.stage('load', 'loading model ' + modelName);

  const net = loadNet(opts, modelName);

  timings.loadMs = ms(t);
  t = now();

  progress.stage('model', 'running ' + modelName + ' at ' + spec.inputSize.join('x'));

  const prob = runSalient(net, spec, bgr);

  timings.modelMs = ms(t);
  t = now();

  let soft = resizeTo(prob, w, h, cv.INTER_CUBIC);

  progress.stage('refine', refine == 'matting' ? 'resolving the edge band with alpha matting' : refine == 'guided' ? 'snapping the matte to the photo edges, guided filter' : 'normalising the matte');

  if(refine == 'guided') soft = refineGuided(bgr, soft);
  else if(refine == 'matting') soft = refineMatting(bgr, soft, opts.workSize ?? 1024);
  else soft = clamp01(soft);

  timings.refineMs = ms(t);
  t = now();

  return cutFromSoft({ opts, img, soft, subjects, minArea, timings, warnings, t0, command: 'cutout', extra: { method: 'dnn', chosen: 'dnn', model: { name: modelName, licence: spec.licence }, refine } });
}

// everything after the soft matte: subjects, edge, colour, files, result.
// c: opts, img, soft (float 0..1 at full size), subjects, minArea, seeds
// (taps in photo px), timings, warnings, t0, command, extra (result keys)
function cutFromSoft(c) {
  const { opts, img, soft, subjects, minArea, timings, warnings } = c;
  const { bgr, width: w, height: h } = img;
  const long = Math.max(w, h);
  let t = now();

  progress.stage('subjects', 'finding the ' + (subjects == 'all' ? 'subjects' : 'main subject'));

  const sub = findSubjects(soft, minArea, subjects, c.seeds);

  if(!sub.chosen.length) fail('E_METHOD', 'the model found no subject above --min-area', { found: sub.found, minArea });

  const grow = guideRadius(long) * 3;
  const parts = sub.chosen.map(p => ({ ...p, ...partGate(sub, p, w, h, grow) }));
  let union = new cv.Mat(h, w, cv.CV_32F, new cv.Scalar(0));

  for(const p of parts) {
    // soft edge inside the gate, solid inside the part itself
    p.alpha = new cv.Mat();
    cv.max(mul(soft, p.gate), p.solid.clone(), p.alpha);
    p.alpha = mul(p.alpha, p.gate);
    cv.max(union, p.alpha, union);
  }

  if(img.alpha) {
    const old = toFloat(img.alpha);

    union = mul(union, old);

    for(const p of parts) p.alpha = mul(p.alpha, old);
  }

  const edgeKey = opts.edgeKey ?? 'auto';
  const strength = opts.edgeStrength ?? 1;

  if(edgeKey != 'none' && strength > 0) {
    const key = edgeKey == 'auto' ? null : (hexColor(edgeKey) ?? fail('E_USAGE', '--edge-key must be auto, none or #rrggbb'));
    const band = opts.edgeBand ?? Math.max(2, Math.round(long * 0.004));
    progress.stage('edge', 'removing background colour at the cut edge');

    const cleaned = defringe(bgr, union, key, band, strength);

    for(const p of parts) cv.min(p.alpha, cleaned, p.alpha);

    union = cleaned;
  }

  timings.subjectsMs = ms(t);
  t = now();

  progress.stage('colour', 'estimating the foreground colour without background');

  const fg = estimateForeground(bgr, union);

  timings.colourMs = ms(t);
  t = now();

  const outputs = [];
  const outlines = [];
  const pad = opts.trimPad ?? 0;
  const place = (file, role, p, index) => {
    let full = bgra(fg, p.alpha);
    let box = { x: 0, y: 0, w, h };

    if(opts.trim) {
      const b = bboxOf(toU8Mat(p.alpha));
      const x = Math.max(0, b.x - pad);
      const y = Math.max(0, b.y - pad);

      box = { x, y, w: Math.min(w, b.x + b.w + pad) - x, h: Math.min(h, b.y + b.h + pad) - y };
      full = full.roi(new cv.Rect(box.x, box.y, box.w, box.h));
    }

    progress.stage('write', 'writing ' + path.basename(file));
    writeFile(file, encode(file, full, opts));

    const a8 = toU8Mat(p.alpha).roi(new cv.Rect(box.x, box.y, box.w, box.h)).clone();
    const solid = new cv.Mat();

    cv.compare(a8, new cv.Scalar(127), solid, cv.CMP_GT);

    const entry = { role, path: file, bbox: box, offset: { x: box.x, y: box.y }, coverage: cv.countNonZero(solid) / (box.w * box.h), score: Number(p.fraction.toFixed(4)) };

    outputs.push(entry);

    if(opts.outlineOut) {
      outlines.push({ output: file, polygon: outline(a8, opts.outlineEpsilon ?? 1.5), epsilon: opts.outlineEpsilon ?? 1.5 });
    }

    return entry;
  };

  const unionPart = { alpha: union, fraction: parts.reduce((s, p) => s + p.fraction, 0) };

  place(opts.out, 'cut', unionPart);

  if(subjects == 'all' && opts.splitDir) {
    try {
      fs.mkdirSync(opts.splitDir, { recursive: true });
    } catch(e) {}

    parts.forEach((p, i) => place(path.join(opts.splitDir, 'cut-' + (i + 1) + '.png'), 'cut', p, i));
  }

  if(opts.restOut) {
    const fill = opts.restFill ?? 'none';

    if(!['none', 'inpaint', 'fsr'].includes(fill)) fail('E_USAGE', '--rest-fill must be none, inpaint or fsr');

    let rest;

    progress.stage('rest', fill == 'none' ? 'building the rest image' : 'filling the hole in the rest image, ' + fill);

    if(fill != 'none') rest = bgra(fillHole(bgr, union, fill == 'fsr' ? 'fsr' : 'telea'), new cv.Mat(h, w, cv.CV_32F, new cv.Scalar(1)));
    else rest = bgra(bgr, oneMinus(union));

    writeFile(opts.restOut, encode(opts.restOut, rest, opts));
    outputs.push({ role: 'rest', path: opts.restOut, fill });
  }

  if(opts.maskOut) writeFile(opts.maskOut, cv.imencode('.png', toU8Mat(union)));
  if(opts.outlineOut) writeFile(opts.outlineOut, JSON.stringify(outlines));
  if(opts.preview) {
    progress.stage('preview', 'writing the preview');
    writePreview(opts.preview, bgr, union);
  }

  timings.writeMs = ms(t);
  timings.totalMs = ms(c.t0);

  const main = outputs[0];

  return {
    ok: true,
    command: c.command,
    ...c.extra,
    input: { path: opts.in, width: w, height: h, channels: img.channels, hasAlpha: img.hasAlpha, orientationApplied: img.orientation },
    output: { path: opts.out, format: path.extname(opts.out).slice(1).toLowerCase(), width: main.bbox.w, height: main.bbox.h, hasAlpha: true, bytes: fs.statSync(opts.out).size },
    mask: { path: opts.maskOut ?? null, coverage: main.coverage, components: parts.length },
    bbox: main.bbox,
    offset: main.offset,
    outputs,
    outlines: opts.outlineOut ? outlines : undefined,
    scores: Object.fromEntries(parts.map((p, i) => ['subject' + (i + 1), Number(p.fraction.toFixed(4))])),
    warnings,
    timings,
  };
}

function cmdInfo(opts) {
  const img = readImage(opts.in, opts.maxPixels);

  return { ok: true, command: 'info', width: img.width, height: img.height, channels: img.channels, hasAlpha: img.hasAlpha, depth: 8, orientationApplied: img.orientation, alphaCoverage: img.alpha ? cv.countNonZero(img.alpha) / (img.width * img.height) : null };
}

function cmdPalette(opts) {
  const img = readImage(opts.in, opts.maxPixels);
  const { bgr, width: w, height: h } = img;
  const dist = ['cube', 'cie76', 'cie94', 'kmeans'].indexOf(opts.dist ?? 'cube');
  const hsv = !!opts.hsv;

  if(dist < 0) fail('E_USAGE', '--dist must be cube, cie76, cie94 or kmeans');

  // work on a reduced copy, the histogram methods do not need more
  const small = workCopy(bgr, 512);
  progress.stage('palette', 'finding the dominant colours');

  const colors = cv.paletteGenerate(small, (hsv ? 1 : 0) | (dist << 1), opts.count || 12);
  const hex = c => '#' + [2, 1, 0].map(i => c[i].toString(16).padStart(2, '0')).join('');

  return { ok: true, command: 'palette', space: hsv ? 'hsv' : 'bgr', input: { width: w, height: h, channels: img.channels }, colors: colors.map(c => (hsv ? [c[0], c[1], c[2]] : hex(c))) };
}

function cmdClip(opts) {
  checkOutPath(opts.out);

  const img = readImage(opts.in, opts.maxPixels);
  const { bgr, width: w, height: h } = img;
  const alpha = img.alpha ?? new cv.Mat(h, w, cv.CV_8UC1, new cv.Scalar(255));
  let polys;

  try {
    polys = JSON.parse(opts.polygon ?? fail('E_USAGE', '--polygon required'));
  } catch(e) {
    if(e instanceof Fail) throw e;

    fail('E_BAD_SHAPE', 'polygon is not valid JSON');
  }

  // supersampled fill, area-downsampled for anti-aliased edges
  progress.stage('clip', 'rasterising the polygon');

  const ss = Math.max(1, Math.min(8, opts.supersample || 4));
  const big = new cv.Mat(h * ss, w * ss, cv.CV_8UC1, new cv.Scalar(0));
  const scale = p => [p[0] * ss, p[1] * ss];

  if(!Array.isArray(polys) || polys.length < 3) fail('E_BAD_SHAPE', 'polygon needs at least 3 points');
  if(!polys.flat(2).every(Number.isFinite)) fail('E_BAD_SHAPE', 'polygon has a non-finite coordinate');

  for(const poly of Array.isArray(polys[0][0]) ? polys : [polys]) cv.fillPoly(big, [poly.map(scale)], [255]);

  const mask = resizeTo(big, w, h, cv.INTER_AREA);

  if(opts.invert) cv.bitwise_not(mask, mask);

  // intersect with any alpha the input already had
  cv.multiply(alpha, mask, alpha, 1 / 255);

  const out = new cv.Mat();

  cv.merge([...cv.split(bgr), alpha], out);
  progress.stage('write', 'writing ' + path.basename(opts.out));
  writeFile(opts.out, encode(opts.out, out, opts));

  return { ok: true, command: 'clip', input: { width: w, height: h, channels: img.channels }, output: { path: opts.out, hasAlpha: true }, mask: { coverage: cv.countNonZero(alpha) / (w * h) } };
}

function cmdCapabilities(opts) {
  const info = cv.getBuildInformation();
  const version = /Version control:\s*(\S+)/.exec(info)?.[1];
  const models = [];
  const unavailable = {};
  const dir = modelsDir(opts);

  const names = Object.entries(MODELS);

  for(const [i, [name, spec]] of names.entries()) {
    progress.stage('probe', 'checking model ' + name);
    progress.sub(i / names.length);

    const entry = { name, kind: spec.kind, licence: spec.licence, commercial: spec.commercial, inputSize: spec.inputSize, loaded: false, unavailable: null };

    for(const part of Object.keys(spec.files)) {
      const file = path.join(dir, spec.files[part]);

      if(!fs.existsSync(file)) entry.unavailable = 'file missing: ' + spec.files[part];
      else if(opts.probe !== false && !entry.unavailable)
        try {
          cv.readNetFromONNX(file, cv.dnn.ENGINE_AUTO);
        } catch(e) {
          entry.unavailable = String(e.message).split('\n')[0];
        }
    }

    entry.loaded = opts.probe !== false && !entry.unavailable;

    if(entry.unavailable) unavailable[name] = entry.unavailable;

    models.push(entry);
  }

  return {
    ok: true,
    tool: 'opencv-imgproc',
    version: VERSION,
    opencv: cv.CV_VERSION_MAJOR + '.' + cv.CV_VERSION_MINOR + '.' + cv.CV_VERSION_REVISION + cv.CV_VERSION_STATUS,
    build: version,
    methods: ['dnn'],
    refine: ['none', 'guided', 'matting'],
    unavailable,
    models,
    modelsDir: dir,
    inputFormats: ['png', 'jpg', 'webp', 'tiff'],
    outputFormats: ['png', 'webp'],
    limits: { maxPixels: 64000000, workSize: 1280 },
  };
}

/* main */

const COMMANDS = { cutout: cmdCutout, prepare: cmdPrepare, segment: cmdSegment, clip: cmdClip, palette: cmdPalette, info: cmdInfo, capabilities: cmdCapabilities };

function main() {
  let result;

  try {
    const { cmd, opts: cli } = parseArgs(process.argv.slice(2));

    if(cli.help || !cmd) {
      emit(process.stdout, { event: 'result', ok: true, command: 'help', usage: USAGE });
      return 0;
    }

    if(!COMMANDS[cmd]) fail('E_USAGE', 'unknown command ' + cmd, { commands: Object.keys(COMMANDS) });

    const opts = loadJob(cli);

    progress.start(cmd, !!opts.progress, PLANS[cmd](opts));
    result = COMMANDS[cmd](opts);
    progress.finish();
  } catch(e) {
    const code = e instanceof Fail ? e.code : 'E_INTERNAL';

    result = { ok: false, error: { code, message: e.message, detail: e.detail } };

    if(!(e instanceof Fail)) log(e.stack ?? String(e));
  }

  emit(process.stdout, { event: 'result', ...result });

  return result.ok ? 0 : EXIT[result.error.code];
}

process.exit(main());

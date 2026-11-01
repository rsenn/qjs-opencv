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

const VERSION = '1.3.0';

const EXIT = { E_USAGE: 2, E_INPUT: 3, E_FORMAT: 4, E_TOO_LARGE: 5, E_BAD_SHAPE: 6, E_MASK_SIZE: 7, E_MODEL: 8, E_METHOD: 9, E_WRITE: 10, E_EMPTY: 11, E_INTERNAL: 70 };

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
// and result events, stderr carries log events. Each line is flushed at once:
// a piped stdout is block buffered, and a phase result must not wait for exit
const emit = (stream, obj) => {
  stream.write(JSON.stringify(obj) + '\n');
  stream.flush?.();
};
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
  phaseName: undefined,
  lo: 0,
  hi: 100,

  start(command, on, plan) {
    Object.assign(this, { on, command, plan, total: plan.reduce((n, [, w]) => n + w, 0) || 1, t0: now(), percent: 0, shown: -1 });
  },

  // a two-phase run: the stages of this phase fill lo..hi percent
  phase(name, lo, hi) {
    Object.assign(this, { phaseName: name, lo, hi, percent: 0, base: 0, weight: 0, shown: -1 });
  },

  report() {
    const percent = Math.round(this.lo + ((this.hi - this.lo) * this.percent) / 100);

    if(!this.on || percent == this.shown && this.message == this.last) return;

    this.shown = percent;
    this.last = this.message;
    emit(process.stdout, { event: 'progress', command: this.command, phase: this.phaseName, stage: this.stage_, message: this.message, percent, elapsedMs: ms(this.t0) });
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
  cutpoly: () => [['read', 30], ['cutpoly', 50], ['write', 20]],
  edgemap: () => [['read', 30], ['edges', 60], ['write', 10]],
  capabilities: () => [['probe', 100]],
  prepare: () => [['read', 8], ['encode', 92]],
  segment: o => [['read', 4], ['encode', 40], ['decode', 4], ['refine', 8], ['subjects', 4], ...(o.edgeKey == 'none' ? [] : [['edge', 8]]), ['colour', 12], ['write', 8], ...(o.restOut ? [['rest', 10]] : []), ...(o.preview ? [['preview', 2]] : [])],
};

const USAGE = `opencv-imgproc.js <command> [options]

  capabilities [--no-probe]
  info    --in FILE
  palette --in FILE [--count 12] [--dist cube|cie76|cie94|kmeans] [--hsv]
  clip    --in FILE --out FILE --polygon JSON [--invert] [--supersample 4]
  cutpoly --in FILE --out FILE --polygon JSON [--mask-out FILE] [--feather 0.0008]
          [--supersample 4]   (normalised 0..1 outlines, even-odd, feathered, RGBA cut)
  edgemap --in FILE --out EDGES.png [--size 640] [--clahe-clip 3] [--clahe-tiles 8]
          [--smooth none|bilateral|guided] [--detector none|hed|dexined]
          [--fuse max|mean] [--detector-weight 0.5] [--contours-out FILE.json]
          [--cache-dir DIR] [--models-dir DIR]   (edge map for the magnetic lasso)
  prepare --in FILE [--model mobilesam|sam] [--cache-dir DIR]
  segment --in FILE --out FILE --point x,y[,label] [--point ...] [--box x,y,w,h]
          [--units px|norm] [--candidates 1..3] [--pick best|0|1|2]
          [--refine none|guided|matting] [--rest-out FILE] [--trim] ...
          [--view-size 1024] [--phases preview,final]
          [--groups single|auto] [--group-gap 0.03] [--mask-prompt off|on]
          [--exclude point|object]
          [--region JSON] [--masks-only]
          [--outline-smooth 0] [--outline-grow 0] [--outline-vertices 0]
          [--soft-slope 2] [--guide-radius 0.006] [--guide-eps 0.01] [--contrast 4]
  cutout  --in FILE --out FILE [--progress] [--method dnn] [--dnn-model u2netp]
          [--refine none|guided|matting] [--subjects largest|all]
          [--split-dir DIR] [--min-area 0.01]
          [--rest-out FILE] [--rest-fill none|inpaint|fsr|lama]
          [--edge-key auto|none|#rrggbb] [--edge-strength 0..1] [--edge-band N]
          [--trim] [--trim-pad N] [--outline-out FILE] [--outline-epsilon 1.5]
          [--mask-out FILE] [--preview FILE] [--models-dir DIR]
          [--view-size 1024] [--phases preview,final]

options are --name value or --name=value, --job FILE.json gives the same
keys as one object, flags override it. every command takes --progress.
all output is NDJSON, the last stdout line is the result event.
with --view-size or --phases, segment and cutout print two result events:
phase "preview" at --view-size px (files <out>.preview.png), then phase
"final" at full size (files <out>), the last line.
spec: doc/opencv-imgproc.md`;

/* option parsing, flag names are the job keys in kebab-case */

const BOOLS = new Set(['trim', 'invert', 'hsv', 'probe', 'help', 'progress', 'masksOnly']);
const LISTS = new Set(['point', 'fg', 'bg']);
const NUMS = new Set(['previewSize', 'viewSize', 'candidates', 'count', 'supersample', 'minArea', 'feather', 'size', 'claheClip', 'claheTiles', 'detectorWeight', 'trimPad', 'outlineEpsilon', 'edgeBand', 'edgeStrength', 'maxPixels', 'workSize', 'seed', 'png', 'softSlope', 'guideRadius', 'guideEps', 'contrast', 'groupGap', 'outlineGrow', 'outlineSmooth', 'outlineVertices']);

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
  // LaMa big-lama, the OpenCV zoo's quantized export (opencv_zoo
  // inpainting_lama_2025jan.onnx): inputs `image` (BGR 0..1) and `mask`
  // (1 = hole), 512x512, output BGR 0..255. Code and weights Apache-2.0,
  // the Places2 training data has terms of its own: check them before shipping
  // edge detectors for edgemap. Neither ships with the repo: export the net to onnx
  // (single 1x1xHxW sigmoid or logit output, the fused side output) and drop it
  // into the models directory. A missing file means detector none, not an error
  hed: {
    kind: 'edge',
    files: { main: 'hed.onnx' },
    inputSize: [512, 512],
    mean: [104.00699, 116.66877, 122.67891],
    scale: 1,
  },
  dexined: {
    kind: 'edge',
    files: { main: 'dexined.onnx' },
    inputSize: [512, 512],
    mean: [103.939, 116.779, 123.68],
    scale: 1,
  },
  lama: {
    kind: 'inpaint',
    files: { main: 'inpainting_lama_2025jan.onnx' },
    sha1: { main: '7c6cdb9362bf73de2a80cfcaf17e121e3302f24c' },
    inputSize: [512, 512],
    licence: 'Apache-2.0 (LaMa), Places2 data terms apply',
    commercial: null,
  },
};

const scriptDir = import.meta.url.replace(/^file:\/\//, '').replace(/[^/]*$/, '');

// a checkout keeps models/ and cache/ beside tools/. An installed copy
// (<prefix>/bin) looks in the user's data dir, then the system ones, and
// caches under the user's cache dir
const inCheckout = fs.existsSync(path.join(scriptDir, '..', 'CMakeLists.txt'));
const homeDir = (env, rel) => env ?? path.join(process.env.HOME ?? '', rel);

function modelsDir(opts) {
  const given = opts.modelsDir ?? process.env.WB_MODELS_DIR;

  if(given) return given;

  const list = inCheckout ? [path.join(scriptDir, '..', 'models')] : [path.join(homeDir(process.env.XDG_DATA_HOME, '.local/share'), 'opencv-imgproc/models'), '/usr/local/share/opencv-imgproc/models', '/usr/share/opencv-imgproc/models'];

  return list.find(d => fs.existsSync(d)) ?? list[0];
}

// a net is loaded once per run: the SAM decoder runs once per tap group and exclude tap, and
// loading it (16 MB) took 125 to 175 ms every time, 1.4 s of a 4-tap selection
const nets = new Map();

function loadNet(opts, name, part = 'main') {
  const spec = MODELS[name] ?? fail('E_MODEL', 'unknown model ' + name, { known: Object.keys(MODELS) });
  const file = path.join(modelsDir(opts), spec.files[part]);

  if(!nets.has(file)) nets.set(file, openNet(file, opts));

  return nets.get(file);
}

function openNet(file, opts) {
  if(!fs.existsSync(file)) fail('E_MODEL', 'model file missing: ' + file, { modelsDir: modelsDir(opts), hint: 'put the weights there, or pass --models-dir / set WB_MODELS_DIR' });

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

// a killed run can leave its temp file behind: drop old ones
function sweepTmp(dir) {
  try {
    for(const f of fs.readdirSync(dir))
      if(/\.tmp-\d+$/.test(f) && Date.now() - fs.statSync(path.join(dir, f)).mtimeMs > 600000) fs.unlinkSync(path.join(dir, f));
  } catch(e) {}
}

// the colour under alpha 0 is never seen, but it is a whole photo to compress: zeroed, a 12 MP cut
// of a small subject is written in 0.5 s instead of 1.2 s and is 2 MB instead of 20
function clearHidden(mat) {
  if(mat.channels() != 4) return mat;

  const planes = [];
  const hidden = new cv.Mat();
  const out = mat.clone();

  cv.split(mat, planes);
  cv.compare(planes[3], new cv.Scalar(0), hidden, cv.CMP_EQ);
  out.setTo(new cv.Scalar(0, 0, 0, 0), hidden);

  return out;
}

function encode(file, mat, opts) {
  const ext = path.extname(file).toLowerCase();

  mat = clearHidden(mat);

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

// every contour of a mask above minShare of the image, biggest first, with
// its hole flag: [{ polygon, hole }]. The mask is first tidied by the options:
// --outline-smooth closes gaps and removes spurs (share of the long edge),
// --outline-grow expands it outwards by that share (the clip of a picture
// whose alpha already cuts it must not cut into the soft edge), and
// --outline-vertices keeps a contour at that many vertices at most by
// raising the simplification tolerance from --outline-epsilon (px)
function outlineAll(alpha8, opts, minShare = 0.0005) {
  const long = Math.max(alpha8.cols, alpha8.rows);
  const epsilon = opts.outlineEpsilon ?? 1.5;
  const bin = new cv.Mat();
  const ellipse = share => {
    const r = Math.max(1, Math.round(share * long));

    return cv.getStructuringElement(cv.MORPH_ELLIPSE, new cv.Size(2 * r + 1, 2 * r + 1));
  };

  cv.threshold(alpha8, bin, 127, 255, cv.THRESH_BINARY);

  if(opts.outlineSmooth > 0) {
    cv.morphologyEx(bin, bin, cv.MORPH_CLOSE, ellipse(opts.outlineSmooth));
    cv.morphologyEx(bin, bin, cv.MORPH_OPEN, ellipse(opts.outlineSmooth));
  }

  if(opts.outlineGrow > 0) cv.dilate(bin, bin, ellipse(opts.outlineGrow));

  const cs = new cv.MatVector();
  const hier = new cv.Mat();

  cv.findContours(bin, cs, hier, cv.RETR_CCOMP, cv.CHAIN_APPROX_NONE);

  const list = [];

  for(let i = 0; i < cs.size(); i++) {
    const area = cv.contourArea(cs.get(i));

    if(area < minShare * alpha8.cols * alpha8.rows) continue;

    const approx = new cv.Mat();
    let eps = epsilon;

    cv.approxPolyDP(cs.get(i), approx, eps, true);

    // a handle per vertex: loosen the tolerance until the contour is few enough
    for(let n = 0; opts.outlineVertices > 0 && approx.rows > opts.outlineVertices && n < 40; n++) cv.approxPolyDP(cs.get(i), approx, (eps *= 1.3), true);

    const polygon = [];

    for(let j = 0; j < approx.data32S.length; j += 2) polygon.push([approx.data32S[j], approx.data32S[j + 1]]);

    if(polygon.length >= 3) list.push({ area, polygon, hole: hier.data32S[i * 4 + 3] >= 0 });
  }

  return list.sort((a, b) => b.area - a.area).map(({ polygon, hole }) => ({ polygon, hole }));
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

// edge constants of the soft matte, segment overrides them with --soft-slope,
// --guide-radius (share of the long side), --guide-eps and --contrast
const TUNE = { softSlope: 2, guideRadius: 0.006, guideEps: 1e-3, contrast: 4 };

// guided filter radius for an image, px
const guideRadius = long => Math.max(4, Math.round(long * TUNE.guideRadius));

// the box around where a soft map is above `level`, grown by `pad` and kept in the picture; null
// when nothing is
function activeBox(soft, level, pad) {
  const on = new cv.Mat();

  cv.compare(soft, new cv.Scalar(level), on, cv.CMP_GT);

  if(!cv.countNonZero(on)) return null;

  const b = cv.boundingRect(on);
  const x0 = Math.max(0, b.x - pad);
  const y0 = Math.max(0, b.y - pad);

  return new cv.Rect(x0, y0, Math.min(soft.cols, b.x + b.width + pad) - x0, Math.min(soft.rows, b.y + b.height + pad) - y0);
}

// snap a soft map to the photo's edges: small guided filter, then steepen. Only around the
// subject: far from it the map is near 0 and the steepening makes it 0 anyway (below 0.375), so a
// small subject in a 12 MP photo costs a fraction of the whole
function refineGuided(bgr, soft) {
  const r = guideRadius(Math.max(bgr.cols, bgr.rows));
  const box = activeBox(soft, 0.05, 3 * r);

  if(!box) return new cv.Mat(soft.rows, soft.cols, cv.CV_32F, new cv.Scalar(0));

  const img = bgr.roi(box).clone();
  const part = soft.roi(box).clone();
  const g = stripwise(img.cols, img.rows, cv.CV_32F, 3 * r, (y0, y1) => {
    const out = new cv.Mat();

    cv.ximgproc.guidedFilter(toFloat(rowsOf(img, y0, y1)), rowsOf(part, y0, y1), out, r, TUNE.guideEps, -1);

    return out;
  });

  if(box.width == soft.cols && box.height == soft.rows) return contrast(g, TUNE.contrast);

  const out = new cv.Mat(soft.rows, soft.cols, cv.CV_32F, new cv.Scalar(0));

  contrast(g, TUNE.contrast).copyTo(out.roi(box));

  return out;
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
// an image operation `fn(bgr, alphaF)` that keeps the size, run only on the box round every
// non-zero alpha widened by `margin` (at least the operation's reach, so every pixel that matters
// sees the same neighbourhood); `outside()` is the full-size result the box is pasted into. A
// subject is mostly a small part of a 12 MP photo, so this saves most of the time
function nearAlpha(bgr, alphaF, margin, fn, outside) {
  const b = bboxOf(toU8Mat(alphaF));
  const x0 = Math.max(0, b.x - margin);
  const y0 = Math.max(0, b.y - margin);
  const x1 = Math.min(alphaF.cols, b.x + b.w + margin);
  const y1 = Math.min(alphaF.rows, b.y + b.h + margin);

  if(!b.w || !b.h || (x1 - x0) * (y1 - y0) > 0.6 * alphaF.cols * alphaF.rows) return fn(bgr, alphaF);

  const rect = new cv.Rect(x0, y0, x1 - x0, y1 - y0);
  const full = outside();

  fn(bgr.roi(rect).clone(), alphaF.roi(rect).clone()).copyTo(full.roi(rect));

  return full;
}

function defringe(bgr, alphaF, key, band, strength) {
  const long = Math.max(bgr.cols, bgr.rows);
  const margin = Math.max(15, Math.round(long * 0.0225)) + 8 * band + 4;

  return nearAlpha(
    bgr,
    alphaF,
    margin,
    (bgr, alphaF) => {
      const live = liveRows(alphaF);

      return stripwise(bgr.cols, bgr.rows, cv.CV_32F, margin, (y0, y1) => (y1 <= live.y0 || y0 >= live.y1 ? rowsOf(alphaF, y0, y1) : defringeCore(toFloat(rowsOf(bgr, y0, y1)), rowsOf(alphaF, y0, y1), key, band, strength, long)));
    },
    () => new cv.Mat(alphaF.rows, alphaF.cols, cv.CV_32F, new cv.Scalar(0)),
  );
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
  const margin = 2 * (r1 + r2) + 2;

  return nearAlpha(
    bgr,
    alphaF,
    margin,
    (bgr, alphaF) => {
      const live = liveRows(alphaF);

      return stripwise(bgr.cols, bgr.rows, cv.CV_8UC3, r1 + r2 + 2, (y0, y1) => (y1 <= live.y0 || y0 >= live.y1 ? rowsOf(bgr, y0, y1) : toU8Mat(fusionCore(toFloat(rowsOf(bgr, y0, y1)), rowsOf(alphaF, y0, y1), r1, r2))));
    },
    () => bgr.clone(),
  );
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

// the hole of a subject filled by LaMa: the photo around the hole (a square
// window with room to see the surroundings) goes through the net at 512x512
// and only the hole is taken from the result, feathered at its edge. The base
// is the Telea fill, it also stands for any part of the hole outside the window
function lamaFill(opts, bgr, alphaF) {
  const base = fillHole(bgr, alphaF, 'inpaint');
  const long = Math.max(bgr.cols, bgr.rows);
  const r = Math.max(2, Math.round(long * 0.008));
  const hole = new cv.Mat();

  cv.threshold(toU8Mat(alphaF), hole, 4, 255, cv.THRESH_BINARY);
  cv.dilate(hole, hole, cv.getStructuringElement(cv.MORPH_ELLIPSE, new cv.Size(2 * r + 1, 2 * r + 1)));

  if(!cv.countNonZero(hole)) return base;

  const b = cv.boundingRect(hole);
  const side = Math.min(Math.min(bgr.cols, bgr.rows), Math.max(256, Math.ceil(Math.max(b.width, b.height) * 1.6)));
  const x = Math.max(0, Math.min(bgr.cols - side, Math.round(b.x + b.width / 2 - side / 2)));
  const y = Math.max(0, Math.min(bgr.rows - side, Math.round(b.y + b.height / 2 - side / 2)));
  const win = new cv.Rect(x, y, side, side);
  const net = loadNet(opts, 'lama');
  const [T] = MODELS.lama.inputSize;
  const how = side > T ? cv.INTER_AREA : cv.INTER_CUBIC;
  const holeWin = hole.roi(win).clone();
  const mask = new cv.Mat();

  cv.threshold(resizeTo(holeWin, T, T, cv.INTER_NEAREST), mask, 127, 255, cv.THRESH_BINARY);
  progress.stage('lama', 'inpainting the hole at ' + T + 'x' + T);

  net.setInput(cv.blobFromImage(resizeTo(base.roi(win).clone(), T, T, how), 1 / 255, new cv.Size(T, T), new cv.Scalar(0, 0, 0), false, false), 'image');
  net.setInput(cv.blobFromImage(mask, 1 / 255, new cv.Size(T, T), new cv.Scalar(0), false, false), 'mask');

  const f = net.forward().data32F;
  const n = T * T;
  const out = new cv.Mat(T, T, cv.CV_8UC3);

  for(let i = 0; i < n; i++) for(let c = 0; c < 3; c++) out.data[i * 3 + c] = Math.max(0, Math.min(255, Math.round(f[c * n + i])));

  const up = resizeTo(out, side, side, cv.INTER_CUBIC);
  const feather = new cv.Mat();
  const h3 = new cv.Mat();

  cv.GaussianBlur(toFloat(holeWin), feather, new cv.Size(0, 0), Math.max(1, r / 2));
  cv.merge([feather, feather, feather], h3);

  const merged = toU8Mat(add(mul(toFloat(base.roi(win).clone()), oneMinus(h3)), mul(toFloat(up), h3)));
  const result = base.clone();

  merged.copyTo(result.roi(win));

  return result;
}

/* promptable segmentation, SAM: the encoder runs once per photo and its
 * output is cached, the decoder runs per tap */

const cacheDir = opts => opts.cacheDir ?? process.env.WB_CACHE_DIR ?? (inCheckout ? path.join(scriptDir, '..', 'cache') : path.join(homeDir(process.env.XDG_CACHE_HOME, '.cache'), 'opencv-imgproc'));

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

  sweepTmp(dir);
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
function samDecode(opts, name, emb, pos, points, scale, prev) {
  const dec = loadNet(opts, name, 'decoder');
  const n = points.length;

  if(MODELS[name].iface == 'orig') return samDecodeOrig(dec, emb, points, scale, prev);

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
function samDecodeOrig(dec, emb, points, scale, prev) {
  const n = points.length + 1;
  const coords = new Float32Array([...points.flatMap(p => [p.x * scale, p.y * scale]), 0, 0]);
  const labels = new Float32Array([...points.map(p => p.label), -1]);

  dec.setInput(emb, 'image_embeddings');
  dec.setInput(ownMat(coords, 1, n * 2, [1, n, 2]), 'point_coords');
  dec.setInput(ownMat(labels, 1, n, [1, n]), 'point_labels');
  // --mask-prompt on: the logits of the previous decode go in as the mask prompt, SAM's own
  // way of taking one more tap on a mask it already found
  dec.setInput(ownMat(prev ? prev.data32F : new Float32Array(256 * 256), 1, 256 * 256, [1, 1, 256, 256]), 'mask_input');
  dec.setInput(ownMat(new Float32Array([prev ? 1 : 0]), 1, 1, [1]), 'has_mask_input');
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

  // sigmoid (slope TUNE.softSlope), the guided filter sharpens the rest
  crop.convertTo(e, cv.CV_32F, -TUNE.softSlope);
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

// the candidate the editor shows for a decode: the whole object, or the part
// when that is most of the photo (the background)
function shownOf(found, pre, T) {
  const area = ((pre.newW * 256) / T) * ((pre.newH * 256) / T);
  const share = f => {
    const m = new cv.Mat();

    cv.compare(f.logits, new cv.Scalar(0), m, cv.CMP_GT);

    return cv.countNonZero(m) / area;
  };

  return found[2] && share(found[2]) <= 0.6 ? found[2] : (found[1] ?? found[0]);
}

// the taps one at a time, every decode gets the mask of the one before as its prompt
function decodeIter(opts, name, e, points, scale, T, pre) {
  let found;

  for(let i = 0; i < points.length; i++) found = samDecode(opts, name, e.emb, e.pos, points.slice(0, i + 1), scale, found ? shownOf(found, pre, T).logits : null);

  return found;
}

// taps one by one: a tap on or next to a group's mask (within `gap` of the
// long side) joins that group, a positive tap elsewhere starts a group of
// its own, a negative one goes to the groups it falls in. every group is
// decoded alone and `found[i].extra` carries the other groups' logits, so
// the matte of candidate i is the union of the groups.
// --exclude object: a negative tap also is decoded alone as an object and its
// mask (`found.cuts`, the largest of its candidates that holds no positive
// tap) is taken out of every candidate. It stays a negative point as well:
// that keeps SAM's "whole" from growing to everything around (a protea and
// its bush), the cut removes what the hint does not (a jet bridge)
function decodeGroups(opts, name, e, points, scale, T, pre) {
  const gap = (opts.groupGap ?? 0.03) * 256;
  const groups = [];
  const cuts = [];
  const cell = p => [Math.min(255, Math.round((p.x * scale * 256) / T)), Math.min(255, Math.round((p.y * scale * 256) / T))];
  const holds = (logits, p) => {
    const [x, y] = cell(p);

    return logits.data32F[y * 256 + x] > 0;
  };
  const positives = points.filter(p => p.label == 1);
  const cutOf = p => {
    const own = samDecode(opts, name, e.emb, e.pos, [{ ...p, label: 1 }], scale);

    return [...own].reverse().find(f => !positives.some(q => holds(f.logits, q)))?.logits ?? null;
  };
  const at = (g, p) => {
    const m = new cv.Mat();
    const inv = new cv.Mat();
    const dist = new cv.Mat();
    const [x, y] = cell(p);

    cv.compare(shownOf(g.found, pre, T).logits, new cv.Scalar(0), m, cv.CMP_GT);
    cv.bitwise_not(m, inv);
    cv.distanceTransform(inv, dist, cv.DIST_L2, 3);

    return dist.data32F[y * 256 + x];
  };
  const decode = g => (g.found = samDecode(opts, name, e.emb, e.pos, g.points, scale, opts.maskPrompt == 'on' && g.found ? shownOf(g.found, pre, T).logits : null));

  for(const p of points) {
    const cut = p.label == 0 && opts.exclude == 'object' ? cutOf(p) : null;

    if(cut) cuts.push(cut);

    const near = groups.filter(g => at(g, p) <= gap);

    if(p.label == 0) {
      for(const g of near) {
        g.points.push(p);
        decode(g);
      }
    } else if(near.length) {
      near[0].points.push(p);
      decode(near[0]);
    } else {
      const g = { points: [p] };

      decode(g);
      groups.push(g);
    }
  }

  // a run of negative taps only has nothing to cut
  if(!groups.length) fail('E_USAGE', 'segment needs a positive point');

  const found = groups[0].found.map((f, i) => ({ index: f.index, score: Math.min(...groups.map(g => g.found[i].score)), logits: f.logits, extra: groups.slice(1).map(g => g.found[i].logits) }));

  found.groups = groups.length;
  found.cuts = cuts;

  return found;
}

// --region JSON: a manual clip as polygons of [x, y] in 0..1 of the photo, the matte is cut to it
function parseRegion(text) {
  if(text === undefined) return null;

  let list;

  try {
    list = JSON.parse(text);
  } catch(e) {
    fail('E_USAGE', '--region must be JSON: a list of polygons of [x, y] in 0..1');
  }

  if(!Array.isArray(list) || !list.length || list.length > 16 || list.some(poly => !Array.isArray(poly) || poly.length < 3 || poly.length > 2000 || poly.some(q => !Array.isArray(q) || !(q[0] >= 0 && q[0] <= 1) || !(q[1] >= 0 && q[1] <= 1)))) fail('E_USAGE', '--region must be a list of 1..16 polygons of 3..2000 points [x, y] in 0..1');

  return list;
}

// 0..1 float mask of the region at w x h
function regionMask(region, w, h) {
  const mask = new cv.Mat(h, w, cv.CV_8UC1, new cv.Scalar(0));

  cv.fillPoly(mask, region.map(poly => poly.map(([x, y]) => [Math.round(x * (w - 1)), Math.round(y * (h - 1))])), new cv.Scalar(255));

  return toFloat(mask);
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

/*
 * two phases (segment, cutout): "preview" runs everything after the model at
 * --view-size px (default 1024) and prints its result at once, "final" then
 * runs the same pipeline at full size from the same model output and prints
 * the last result line. The preview files are <out>.preview.<ext> (same for
 * --mask-out and --outline-out; --rest-out, --preview and --split-dir are
 * final only). --view-size alone means preview,final; without either there is
 * one result, as before.
 */
function phasesOf(opts) {
  const text = opts.phases ?? (opts.viewSize ? 'preview,final' : undefined);

  if(text === undefined) return null;

  const list = String(text).split(',').map(x => x.trim());

  if(!['preview', 'final', 'preview,final'].includes(list.join(','))) fail('E_USAGE', '--phases must be preview, final or preview,final');

  return list;
}

const viewSizeOf = opts => opts.viewSize ?? opts.previewSize ?? 1024;
const phasePath = (file, phase) => (file && phase == 'preview' ? file.replace(/(\.[^./]+)$/, '.preview$1') : file);

const phaseOpts = (opts, phase) =>
  phase == 'preview' ? { ...opts, out: phasePath(opts.out, phase), maskOut: phasePath(opts.maskOut, phase), outlineOut: phasePath(opts.outlineOut, phase), restOut: undefined, preview: undefined, splitDir: undefined } : opts;

// the photo at tier scale k (a view copy, or the photo itself)
function tierImage(full, k) {
  if(k >= 1) return full;

  const size = Math.round(Math.max(full.width, full.height) * k);
  const img = { ...full, bgr: workCopy(full.bgr, size), alpha: full.alpha && workCopy(full.alpha, size) };

  img.width = img.bgr.cols;
  img.height = img.bgr.rows;

  return img;
}

// run the phases in order, all but the last print their result here
function runPhases(opts, full, render) {
  const phases = phasesOf(opts);
  const long = Math.max(full.width, full.height);

  if(!phases) return render(opts.previewSize && opts.previewSize < long ? opts.previewSize / long : 1, opts, undefined);

  for(const [i, phase] of phases.entries()) {
    if(i > 0) progress.phase(phase, 30, 100);

    const result = render(phase == 'preview' ? Math.min(1, viewSizeOf(opts) / long) : 1, phaseOpts(opts, phase), phase);

    if(i == phases.length - 1) return result;

    emit(process.stdout, { event: 'result', ...result });
  }
}

function cmdSegment(opts) {
  const t0 = now();
  const name = samModel(opts);
  const spec = MODELS[name];
  const refine = opts.refine ?? 'guided';
  const candidates = Math.max(1, Math.min(3, opts.candidates ?? 1));
  const phases = phasesOf(opts);

  // segment's own default, cutout keeps 1e-3 (tools/tuning: 0.01 lifts boundary F by 0.04)
  TUNE.guideEps = 0.01;

  for(const k of Object.keys(TUNE)) if(opts[k] !== undefined) TUNE[k] = opts[k];

  if(!opts.masksOnly) checkOutPath(opts.out);

  if(opts.exclude !== undefined && !['point', 'object'].includes(opts.exclude)) fail('E_USAGE', '--exclude must be point or object');

  if(opts.restOut) checkOutPath(opts.restOut, '--rest-out');
  if(!['none', 'guided', 'matting'].includes(refine)) fail('E_USAGE', '--refine must be none, guided or matting');
  if(opts.pick !== undefined && opts.pick != 'best' && !['0', '1', '2'].includes(String(opts.pick))) fail('E_USAGE', '--pick must be best, 0, 1 or 2');

  if(phases) progress.phase(phases[0], 0, phases.length == 1 ? 100 : 30);

  const full = readImage(opts.in, opts.maxPixels);
  const { points, box } = parsePrompts(opts, full.width, full.height);

  // only exclude taps select nothing: SAM given just "not here" makes up a speck, and an apply
  // would have nothing to cut
  if(!points.some(p => p.label == 1)) {
    if(opts.masksOnly) return { ok: true, command: 'segment', phase: 'select', scale: 1, original: { width: full.width, height: full.height }, output: { width: full.width, height: full.height }, candidates: [], groups: 0, excluded: 0, picked: null, best: null, prompts: points, timings: { totalMs: ms(t0) } };

    fail('E_USAGE', 'segment needs a positive point');
  }
  const region = parseRegion(opts.region);
  const e = samEmbeddings(opts, full, name);
  const [T] = spec.inputSize;
  const pre = { newW: e.meta.newW, newH: e.meta.newH };
  const t = now();

  progress.stage('decode', 'segmenting at ' + points.length + (points.length == 1 ? ' prompt' : ' prompts'));

  // the decoder takes taps in photo px: its logits do not depend on the tier
  const scale = T / Math.max(full.width, full.height);
  const found = opts.groups == 'auto' && points.length > 1 ? decodeGroups(opts, name, e, points, scale, T, pre) : opts.maskPrompt == 'on' && points.length > 1 ? decodeIter(opts, name, e, points, scale, T, pre) : samDecode(opts, name, e.emb, e.pos, points, scale);
  const base = { prepareMs: e.ms, decodeMs: ms(t) };

  const ctx = { name, spec, refine, candidates, full, points, box, region, e, found, T, pre, base, t0 };

  if(opts.masksOnly) return renderSegment(ctx, Math.min(1, viewSizeOf(opts) / Math.max(full.width, full.height)), opts, 'select');

  return runPhases(opts, full, (k, o, phase) => renderSegment(ctx, k, o, phase));
}

// a soft matte as the wand shows it: the parts under a positive tap (else the largest) as tidy
// outlines in pixels of the matte, and their share of it. Nothing found is no polygons, not an error
function maskOutline(soft, opts, seeds) {
  const sub = findSubjects(soft, opts.minArea ?? 0.0002, 'largest', seeds);
  const mask = new cv.Mat(sub.small.rows, sub.small.cols, cv.CV_8UC1, new cv.Scalar(0));

  for(const p of sub.chosen) {
    const m = new cv.Mat();

    cv.compare(sub.labels, new cv.Scalar(p.label), m, cv.CMP_EQ);
    cv.bitwise_or(mask, m, mask);
  }

  const full = mask.cols == soft.cols && mask.rows == soft.rows ? mask : resizeTo(mask, soft.cols, soft.rows, cv.INTER_NEAREST);

  return { coverage: cv.countNonZero(full) / (full.cols * full.rows), polygons: sub.chosen.length ? outlineAll(full, opts) : [] };
}

// refine, subjects, colour and files for every candidate at tier scale k
function renderSegment(ctx, k, o, phase) {
  const { name, spec, refine, candidates, full, found, T, pre, e, t0 } = ctx;
  const tp = now();
  const timings = { ...ctx.base };
  const warnings = [];
  const img = tierImage(full, k);
  const { bgr, width: w, height: h } = img;
  const points = ctx.points.map(p => ({ ...p, x: p.x * k, y: p.y * k }));
  const box = ctx.box && { x: ctx.box.x * k, y: ctx.box.y * k, w: ctx.box.w * k, h: ctx.box.h * k };

  const best = found.reduce((a, b) => (b.score > a.score ? b : a)).index;
  const fit = box ? boxFit(found, box, Math.max(w, h)) : null;
  const picked = o.pick === undefined || o.pick == 'best' ? (fit?.index ?? best) : Number(o.pick);
  const wanted = candidates > 1 ? found.slice(0, candidates).map(f => f.index) : [picked];
  // masks-only writes no file, its names are never used
  const stem = (o.out ?? 'cut.png').replace(/\.[^.]+$/, '');
  const ext = path.extname(o.out ?? 'cut.png');
  const taps = points.filter(p => p.label == 1).map(p => [p.x, p.y]);
  const list = [];
  let main;
  let t;

  if(!wanted.includes(picked)) wanted.push(picked);

  // the soft matte of candidate `index`: the union of the groups, refined, cut to the region
  const softOf = index => {
    let soft = samSoft(found[index].logits, T, pre, w, h);

    for(const l of found[index].extra ?? []) cv.max(soft, samSoft(l, T, pre, w, h), soft);

    // --exclude object: what the negative taps found is taken out
    for(const l of found.cuts ?? []) soft = mul(soft, oneMinus(samSoft(l, T, pre, w, h)));

    if(refine == 'guided') soft = refineGuided(bgr, soft);
    else if(refine == 'matting') soft = refineMatting(bgr, soft, o.workSize ?? 1024);
    else soft = clamp01(soft);

    return ctx.region ? mul(soft, regionMask(ctx.region, w, h)) : soft;
  };

  // --masks-only (the wand's select step): every candidate as outlines at this size, no cut, no files
  if(o.masksOnly) {
    const list = found.slice(0, candidates).map(f => {
      progress.stage('refine', 'outlining candidate ' + f.index);

      return { index: f.index, score: Number(f.score.toFixed(4)), ...maskOutline(softOf(f.index), o, taps) };
    });

    timings.totalMs = ms(t0);

    return { ok: true, command: 'segment', phase: 'select', scale: k, original: { width: full.width, height: full.height }, output: { width: w, height: h }, cache: e.hit ? 'hit' : 'miss', model: { name, licence: spec.licence }, candidates: list, groups: found.groups, excluded: found.cuts?.length ?? 0, picked, best, prompts: points, timings };
  }

  for(const index of [picked, ...wanted.filter(i => i != picked)]) {
    const isMain = index == picked;
    const file = isMain ? o.out : stem + '-' + index + ext;

    t = now();
    progress.stage('refine', 'refining candidate ' + index + ', ' + (refine == 'matting' ? 'alpha matting' : 'guided filter'));

    let soft = softOf(index);

    const own = isMain ? o : { ...o, out: file, restOut: undefined, maskOut: undefined, outlineOut: undefined, preview: undefined, splitDir: undefined };
    const result = cutFromSoft({ opts: own, img, soft, subjects: 'largest', minArea: o.minArea ?? 0.0002, seeds: taps, timings: isMain ? timings : {}, warnings: isMain ? warnings : [], t0, command: 'segment', extra: {} });

    list.push({ index, score: Number(found[index].score.toFixed(4)), path: file, picked: isMain, coverage: result.mask.coverage, bbox: result.bbox });
    if(isMain) main = result;
  }

  // with several candidates every one has its own file, the picked one also is out
  if(candidates > 1) {
    const own = stem + '-' + picked + ext;

    writeFile(own, fs.readFileSync(o.out));
    list.find(c => c.index == picked).path = own;
  }

  list.sort((a, b) => a.index - b.index);
  timings.totalMs = ms(t0);

  if(phase) timings.phaseMs = ms(tp);

  return { ...main, phase, scale: k, original: { width: full.width, height: full.height }, view: { width: w, height: h }, cache: e.hit ? 'hit' : 'miss', model: { name, licence: spec.licence }, refine, candidates: list, groups: found.groups, excluded: found.cuts?.length ?? 0, picked, best, boxIou: fit ? Number(fit.iou.toFixed(4)) : undefined, prompts: points, timings };
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
  const phases = phasesOf(opts);

  checkOutPath(opts.out);

  if(!['auto', 'dnn'].includes(method)) fail('E_METHOD', 'method ' + method + ' is not available, use dnn', { available: ['dnn'] });
  if(!['none', 'guided', 'matting'].includes(refine)) fail('E_USAGE', '--refine must be none, guided or matting');
  if(!['largest', 'all'].includes(subjects)) fail('E_USAGE', '--subjects must be largest or all');
  if(opts.restOut) checkOutPath(opts.restOut, '--rest-out');
  if(subjects == 'all' && !opts.splitDir) warnings.push('--subjects all without --split-dir: only the union is written');

  cv.setRNGSeed(opts.seed ?? 0);

  const spec = MODELS[modelName];

  if(spec?.kind != 'salient') fail('E_MODEL', modelName + ' is not a salient-object model', { salient: Object.keys(MODELS).filter(k => MODELS[k].kind == 'salient') });

  if(phases) progress.phase(phases[0], 0, phases.length == 1 ? 100 : 30);

  let t = now();
  const img = readImage(opts.in, opts.maxPixels);

  timings.readMs = ms(t);
  t = now();

  progress.stage('load', 'loading model ' + modelName);

  const net = loadNet(opts, modelName);

  timings.loadMs = ms(t);
  t = now();

  progress.stage('model', 'running ' + modelName + ' at ' + spec.inputSize.join('x'));

  // the model sees 320 px whatever the tier: both phases use this one matte
  const prob = runSalient(net, spec, img.bgr);

  timings.modelMs = ms(t);

  return runPhases(opts, img, (k, o, phase) => renderCutout({ img, prob, spec, modelName, refine, subjects, minArea, base: timings, warnings, t0 }, k, o, phase));
}

// refine, subjects, colour and files at tier scale k
function renderCutout(ctx, k, o, phase) {
  const { prob, spec, modelName, refine, subjects, minArea, t0 } = ctx;
  const tp = now();
  const timings = { ...ctx.base };
  const img = tierImage(ctx.img, k);
  const { bgr, width: w, height: h } = img;
  let t = now();
  let soft = resizeTo(prob, w, h, cv.INTER_CUBIC);

  progress.stage('refine', refine == 'matting' ? 'resolving the edge band with alpha matting' : refine == 'guided' ? 'snapping the matte to the photo edges, guided filter' : 'normalising the matte');

  if(refine == 'guided') soft = refineGuided(bgr, soft);
  else if(refine == 'matting') soft = refineMatting(bgr, soft, o.workSize ?? 1024);
  else soft = clamp01(soft);

  timings.refineMs = ms(t);

  const tier = phase ? { phase, scale: k, original: { width: ctx.img.width, height: ctx.img.height }, view: { width: w, height: h } } : {};
  const result = cutFromSoft({ opts: o, img, soft, subjects, minArea, timings, warnings: [...ctx.warnings], t0, command: 'cutout', extra: { method: 'dnn', chosen: 'dnn', model: { name: modelName, licence: spec.licence }, refine, ...tier } });

  if(phase) result.timings.phaseMs = ms(tp);

  return result;
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

    // polygon in the pixels of that output file; --outline-out also writes them
    const polygons = outlineAll(a8, opts);

    // `polygon` is the biggest object, `polygons` every object and hole
    outlines.push({ output: file, polygon: polygons.find(c => !c.hole)?.polygon ?? outline(a8, opts.outlineEpsilon ?? 1.5), polygons, epsilon: opts.outlineEpsilon ?? 1.5 });

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

    if(!['none', 'inpaint', 'fsr', 'lama'].includes(fill)) fail('E_USAGE', '--rest-fill must be none, inpaint, fsr or lama');

    let rest;

    progress.stage('rest', fill == 'none' ? 'building the rest image' : 'filling the hole in the rest image, ' + fill);

    if(fill == 'lama') rest = bgra(lamaFill(opts, bgr, union), new cv.Mat(h, w, cv.CV_32F, new cv.Scalar(1)));
    else if(fill != 'none') rest = bgra(fillHole(bgr, union, fill == 'fsr' ? 'fsr' : 'telea'), new cv.Mat(h, w, cv.CV_32F, new cv.Scalar(1)));
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
    bboxNorm: { x: main.bbox.x / w, y: main.bbox.y / h, w: main.bbox.w / w, h: main.bbox.h / h },
    offset: main.offset,
    outputs,
    outlines,
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

/* cutpoly: cut the photo along a finished polygon, e.g. the editor's magnetic lasso.
 * --polygon is one outline [[x,y],...] or a list of outlines, x,y in 0..1 of the
 * whole photo (after the exif turn). Outlines combine even-odd, so one inside
 * another is a hole. The fill is supersampled, area-downscaled, then feathered
 * with a gaussian of sigma max(0.5, feather * long edge px). Only the bounding
 * box of the outlines (plus the blur reach) is rasterised, so a 12 MP photo
 * with a small outline costs little. */
function cmdCutpoly(opts) {
  checkOutPath(opts.out);
  if(opts.maskOut) checkOutPath(opts.maskOut, '--mask-out');

  const img = readImage(opts.in, opts.maxPixels);
  const { bgr, width: w, height: h } = img;
  let polys;

  try {
    polys = JSON.parse(opts.polygon ?? fail('E_USAGE', '--polygon required'));
  } catch(e) {
    if(e instanceof Fail) throw e;

    fail('E_BAD_SHAPE', 'polygon is not valid JSON');
  }

  if(!Array.isArray(polys) || !polys.length) fail('E_BAD_SHAPE', 'polygon needs at least 3 points');
  if(!Array.isArray(polys[0]?.[0])) polys = [polys];
  if(!polys.every(p => Array.isArray(p) && p.length >= 3 && p.every(q => Array.isArray(q) && q.length >= 2 && Number.isFinite(q[0]) && Number.isFinite(q[1])))) fail('E_BAD_SHAPE', 'every outline needs at least 3 finite [x,y] points');

  progress.stage('cutpoly', 'rasterising the polygon');

  const ss = Math.max(1, Math.min(8, Math.round(opts.supersample || 4)));
  const sigma = Math.max(0.5, (opts.feather ?? 0.0008) * Math.max(w, h));
  const pad = Math.ceil(sigma * 3) + 2;
  const px = polys.map(p => p.map(q => [q[0] * w, q[1] * h]));

  // window of the pixels the mask can reach
  let x0 = Infinity, y0 = Infinity, x1 = -Infinity, y1 = -Infinity;

  for(const p of px)
    for(const [x, y] of p) {
      x0 = Math.min(x0, x);
      y0 = Math.min(y0, y);
      x1 = Math.max(x1, x);
      y1 = Math.max(y1, y);
    }

  const wx0 = Math.max(0, Math.floor(x0) - pad);
  const wy0 = Math.max(0, Math.floor(y0) - pad);
  const wx1 = Math.min(w, Math.ceil(x1) + pad);
  const wy1 = Math.min(h, Math.ceil(y1) + pad);
  const mask = new cv.Mat(h, w, cv.CV_8UC1, new cv.Scalar(0));

  if(wx1 > wx0 && wy1 > wy0) {
    const ww = wx1 - wx0;
    const wh = wy1 - wy0;
    const big = new cv.Mat(wh * ss, ww * ss, cv.CV_8UC1, new cv.Scalar(0));

    for(const p of px) {
      const one = new cv.Mat(wh * ss, ww * ss, cv.CV_8UC1, new cv.Scalar(0));

      cv.fillPoly(one, [p.map(([x, y]) => [Math.round((x - wx0) * ss), Math.round((y - wy0) * ss)])], [255]);
      cv.bitwise_xor(big, one, big);
    }

    // a degenerate outline (identical or collinear points) still lights a few subpixels:
    // under half a photo pixel of coverage counts as empty
    if(cv.countNonZero(big) < 0.5 * ss * ss) fail('E_EMPTY', 'empty');

    const win = resizeTo(big, ww, wh, cv.INTER_AREA);
    const soft = new cv.Mat();

    cv.GaussianBlur(win, soft, new cv.Size(0, 0), sigma);
    soft.copyTo(mask.roi(new cv.Rect(wx0, wy0, ww, wh)));
  }

  if(cv.countNonZero(mask) == 0) fail('E_EMPTY', 'empty');

  const out = new cv.Mat();

  cv.merge([...cv.split(bgr), mask], out);
  progress.stage('write', 'writing ' + path.basename(opts.out));
  writeFile(opts.out, encode(opts.out, out, opts));
  if(opts.maskOut) writeFile(opts.maskOut, encode(opts.maskOut, mask, opts));

  return { ok: true, command: 'cutpoly', output: { path: path.resolve(opts.out), width: w, height: h }, ...(opts.maskOut ? { maskPath: path.resolve(opts.maskOut) } : {}), coverage: cv.countNonZero(mask) / (w * h) };
}

/* edgemap: an edge map of the photo for the editor's magnetic lasso, whose live wire
 * (Dijkstra over a per-pixel cost) runs in the browser. Pipeline:
 *   1. scale to --size px long edge (never up), INTER_AREA
 *   2. CLAHE on the Lab lightness only (cv.createCLAHE), so a dim photo still gets edges
 *   3. edge-aware smoothing of the Lab image: bilateral (default), guided, or none
 *   4. Sobel L2 gradient (lightness, plus chroma at half weight) fused with Canny whose
 *      thresholds come from Otsu of the gradient. strength = max(sobel, soft canny):
 *      thin connected edges win over noisy gradients. The gradient is normalised by its
 *      98th percentile (floored, so a photo with few edges is not blown up) after
 *      subtracting a noise floor of twice the median, so flat areas land at 0
 *   5. optionally a dnn edge detector (hed, dexined) fused by max or weighted mean
 * the result is 8-bit, one channel, same size as the scaled copy, 255 = strongest edge */

const EDGE_FLOOR = 64; // sobel units: a step of 16 grey levels is 64

const sampleOf = (m, n = 100000) => {
  const d = m.data32F;
  const step = Math.max(1, Math.floor(d.length / n));
  const out = new Float32Array(Math.ceil(d.length / step));

  for(let i = 0, j = 0; i < d.length; i += step) out[j++] = d[i];

  return out.sort();
};

const pctOf = (sorted, p) => sorted[Math.min(sorted.length - 1, Math.floor(p * sorted.length))];

// otsu threshold of values in 0..top over 128 bins
function otsuOf(sorted, top) {
  const N = 128;
  const hist = new Float64Array(N);

  for(const v of sorted) hist[Math.min(N - 1, Math.floor((v / top) * N))]++;

  const total = sorted.length;
  let sum = 0;

  for(let i = 0; i < N; i++) sum += i * hist[i];

  let wB = 0, sumB = 0, best = 0, bestT = 0;

  for(let t = 0; t < N; t++) {
    wB += hist[t];

    if(!wB) continue;

    const wF = total - wB;

    if(!wF) break;

    sumB += t * hist[t];

    const d = (wB * wF * (sumB / wB - (sum - sumB) / wF) ** 2);

    if(d > best) {
      best = d;
      bestT = t;
    }
  }

  return ((bestT + 1) / N) * top;
}

// the network edge map at the working size in 0..1, or null with a reason
function netEdges(opts, name, bgr) {
  const spec = MODELS[name];
  const file = path.join(modelsDir(opts), spec.files.main);

  if(!fs.existsSync(file)) return { note: `${spec.files.main} not found in ${modelsDir(opts)}` };

  let out;

  try {
    const net = loadNet(opts, name);
    const [T] = spec.inputSize;

    net.setInput(cv.blobFromImage(resizeTo(bgr, T, T, cv.INTER_AREA), spec.scale, new cv.Size(T, T), new cv.Scalar(...spec.mean), false, false));

    const o = net.forward();
    const d = o.data32F;
    const f = new cv.Mat(T, T, cv.CV_32FC1);
    const logit = d.some(v => v < 0 || v > 1);

    for(let i = 0; i < T * T; i++) f.data32F[i] = logit ? 1 / (1 + Math.exp(-d[i])) : d[i];

    out = resizeTo(f, bgr.cols, bgr.rows, cv.INTER_LINEAR);
  } catch(e) {
    return { note: `${spec.files.main} failed: ${String(e.message).split('\n')[0]}` };
  }

  return { map: out };
}

function edgeMap(bgr, opts) {
  const detector = opts.detector ?? 'none';
  const smooth = opts.smooth ?? 'bilateral';
  const lab = new cv.Mat();
  const planes = [];

  cv.cvtColor(bgr, lab, cv.COLOR_BGR2Lab);
  cv.split(lab, planes);

  const L = new cv.Mat();

  cv.createCLAHE(opts.claheClip ?? 3, new cv.Size(opts.claheTiles ?? 8, opts.claheTiles ?? 8)).apply(planes[0], L);
  cv.merge([L, planes[1], planes[2]], lab);

  let sm = lab;

  if(smooth == 'bilateral') {
    sm = new cv.Mat();
    cv.bilateralFilter(lab, sm, 5, 30, 5);
  } else if(smooth == 'guided') {
    const f = new cv.Mat();
    const g = new cv.Mat();

    lab.convertTo(f, cv.CV_32F, 1 / 255);
    cv.ximgproc.guidedFilter(f, f, g, Math.max(2, Math.round(0.004 * Math.max(lab.cols, lab.rows))), 0.003, -1);
    sm = new cv.Mat();
    g.convertTo(sm, cv.CV_8U, 255);
  }

  const chans = [];

  cv.split(sm, chans);

  const grad = ch => {
    const gx = new cv.Mat();
    const gy = new cv.Mat();
    const m = new cv.Mat();

    cv.Sobel(ch, gx, cv.CV_32F, 1, 0, 3);
    cv.Sobel(ch, gy, cv.CV_32F, 0, 1, 3);
    cv.magnitude(gx, gy, m);

    return m;
  };

  const mag = grad(chans[0]);

  for(const c of [1, 2]) {
    const m = grad(chans[c]);

    m.convertTo(m, cv.CV_32F, 0.5);
    cv.max(mag, m, mag);
  }

  const sorted = sampleOf(mag);
  const noise = 2 * pctOf(sorted, 0.5);
  const top = Math.max(pctOf(sorted, 0.98), EDGE_FLOOR);
  const denom = Math.max(1, top - noise);
  const sobel = new cv.Mat();

  mag.convertTo(sobel, cv.CV_32F, 1 / denom, -noise / denom);
  cv.threshold(sobel, sobel, 0, 0, cv.THRESH_TOZERO);
  cv.threshold(sobel, sobel, 1, 1, cv.THRESH_TRUNC);

  // canny on the smoothed lightness, thresholds from otsu of the gradient
  const cap = Math.max(pctOf(sorted, 0.99), 1);
  const hi = Math.max(otsuOf(sorted, cap), noise * 1.5, 24);
  const canny = new cv.Mat();

  cv.Canny(chans[0], canny, hi * 0.4, hi, 3, true);
  cv.dilate(canny, canny, cv.getStructuringElement(cv.MORPH_RECT, new cv.Size(3, 3)));

  const soft = new cv.Mat();

  canny.convertTo(soft, cv.CV_32F, 1 / 255);
  cv.GaussianBlur(soft, soft, new cv.Size(3, 3), 0.8);
  soft.convertTo(soft, cv.CV_32F, 1.25); // the blur takes the peak to about 0.8

  let fused = new cv.Mat();

  cv.max(sobel, soft, fused);
  cv.threshold(fused, fused, 1, 1, cv.THRESH_TRUNC);

  let used = 'none';
  let note;

  if(detector != 'none') {
    if(!MODELS[detector] || MODELS[detector].kind != 'edge') fail('E_USAGE', '--detector must be none, hed or dexined');

    const net = netEdges(opts, detector, bgr);

    if(net.map) {
      const wgt = opts.detectorWeight ?? 0.5;
      const w = new cv.Mat();

      if((opts.fuse ?? 'max') == 'mean') cv.addWeighted(fused, 1 - wgt, net.map, wgt, 0, fused);
      else {
        net.map.convertTo(w, cv.CV_32F, Math.min(1, wgt * 2));
        cv.max(fused, w, fused);
      }

      used = detector;
    } else note = net.note;
  }

  const out = new cv.Mat();

  fused.convertTo(out, cv.CV_8U, 255);

  return { map: out, detector: used, note };
}

// strongest closed contours of the map, simplified, in 0..1 of the map
function edgeContours(map, epsilon = 1.5, keep = 64) {
  const bin = new cv.Mat();
  const cs = new cv.MatVector();
  const { cols: w, rows: h } = map;

  cv.threshold(map, bin, 95, 255, cv.THRESH_BINARY);
  cv.findContours(bin, cs, new cv.Mat(), cv.RETR_LIST, cv.CHAIN_APPROX_NONE);

  const list = [];

  for(let i = 0; i < cs.size(); i++) {
    const c = cs.get(i);
    const length = cv.arcLength(c, true);

    if(length < 20) continue;

    const approx = new cv.Mat();

    cv.approxPolyDP(c, approx, epsilon, true);

    const d = approx.data32S;
    const points = [];
    let sum = 0;

    for(let j = 0; j < d.length; j += 2) {
      points.push([+(d[j] / w).toFixed(5), +(d[j + 1] / h).toFixed(5)]);
      sum += map.data[Math.min(h - 1, d[j + 1]) * w + Math.min(w - 1, d[j])];
    }

    list.push({ length: Math.round(length), strength: +(sum / points.length / 255).toFixed(3), closed: true, points });
  }

  return list.sort((a, b) => b.length * b.strength - a.length * a.strength).slice(0, keep);
}

function cmdEdgemap(opts) {
  checkOutPath(opts.out);

  const t0 = now();
  const size = Math.max(32, Math.round(opts.size || 640));
  const smooth = opts.smooth ?? 'bilateral';
  const detector = opts.detector ?? 'none';
  const fuse = opts.fuse ?? 'max';

  if(!['none', 'bilateral', 'guided'].includes(smooth)) fail('E_USAGE', '--smooth must be none, bilateral or guided');
  if(!['none', 'hed', 'dexined'].includes(detector)) fail('E_USAGE', '--detector must be none, hed or dexined');
  if(!['max', 'mean'].includes(fuse)) fail('E_USAGE', '--fuse must be max or mean');

  const args = { size, claheClip: opts.claheClip ?? 3, claheTiles: Math.max(1, Math.round(opts.claheTiles ?? 8)), smooth, detector, fuse, detectorWeight: opts.detectorWeight ?? 0.5 };

  let u8;

  try {
    u8 = toU8(fs.readFileSync(opts.in ?? fail('E_USAGE', 'missing --in')));
  } catch(e) {
    if(e instanceof Fail) throw e;

    fail('E_INPUT', 'cannot read ' + opts.in);
  }

  // the cache key covers the photo bytes, every flag, and the detector's weights file
  let weights = '';

  if(detector != 'none') {
    try {
      const st = fs.statSync(path.join(modelsDir(opts), MODELS[detector].files.main));

      weights = st.size + '@' + st.mtimeMs;
    } catch(e) {}
  }

  const key = cv.hash.sha256(u8) + '-' + cv.hash.sha256(JSON.stringify({ v: 1, ...args, weights, contours: !!opts.contoursOut })).slice(0, 16);
  const dir = cacheDir(opts);
  const base = path.join(dir, 'edgemap-' + key);
  const meta = base + '.json';
  const done = (output, extra) => ({ ok: true, command: 'edgemap', output, ...extra, timings: { totalMs: ms(t0) } });

  if(fs.existsSync(base + '.png') && fs.existsSync(meta) && (!opts.contoursOut || fs.existsSync(base + '.contours.json'))) {
    try {
      const m = JSON.parse(fs.readFileSync(meta, 'utf8'));

      writeFile(opts.out, toU8(fs.readFileSync(base + '.png')));
      if(opts.contoursOut) writeFile(opts.contoursOut, fs.readFileSync(base + '.contours.json', 'utf8'));

      return done({ path: path.resolve(opts.out), width: m.width, height: m.height }, { detector: m.detector, ...(m.note ? { detectorNote: m.note } : {}), cached: true });
    } catch(e) {
      // unreadable cache entry: recompute
    }
  }

  const img = readImage(opts.in, opts.maxPixels);

  progress.stage('edges', 'scaling and finding edges');

  const k = Math.min(1, size / Math.max(img.width, img.height));
  const work = k < 1 ? resizeTo(img.bgr, Math.max(1, Math.round(img.width * k)), Math.max(1, Math.round(img.height * k)), cv.INTER_AREA) : img.bgr;
  const { map, detector: used, note } = edgeMap(work, args.detector == 'none' ? { ...args } : { ...args, modelsDir: opts.modelsDir });
  const bytes = encode(opts.out, map, opts);

  progress.stage('write', 'writing ' + path.basename(opts.out));
  writeFile(opts.out, bytes);

  let contours;

  if(opts.contoursOut) {
    contours = JSON.stringify({ width: map.cols, height: map.rows, contours: edgeContours(map) });
    writeFile(opts.contoursOut, contours);
  }

  try {
    fs.mkdirSync(dir, { recursive: true });
    sweepTmp(dir);
    writeFile(base + '.png', bytes);
    if(contours) writeFile(base + '.contours.json', contours);
    writeFile(meta, JSON.stringify({ width: map.cols, height: map.rows, detector: used, note }));
  } catch(e) {
    // a cache that cannot be written is not an error
  }

  return done({ path: path.resolve(opts.out), width: map.cols, height: map.rows }, { detector: used, ...(note ? { detectorRequested: detector, detectorNote: note } : {}), cached: false });
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
    phases: ['preview', 'final'],
    limits: { maxPixels: 64000000, workSize: 1280, viewSize: 1024 },
  };
}

/* main */

const COMMANDS = { cutout: cmdCutout, prepare: cmdPrepare, segment: cmdSegment, clip: cmdClip, cutpoly: cmdCutpoly, edgemap: cmdEdgemap, palette: cmdPalette, info: cmdInfo, capabilities: cmdCapabilities };

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

    result = code == 'E_EMPTY' ? { ok: false, command: 'cutpoly', error: 'empty' } : { ok: false, phase: progress.phaseName, error: { code, message: e.message, detail: e.detail } };

    if(!(e instanceof Fail)) log(e.stack ?? String(e));
  }

  emit(process.stdout, { event: 'result', ...result });

  return result.ok ? 0 : EXIT[result.error.code ?? 'E_EMPTY'];
}

process.exit(main());

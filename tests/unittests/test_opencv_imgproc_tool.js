import { tests, assert, eq as strictEq } from './tinytest.js';
import * as cv from 'opencv';
import * as fs from 'fs';
import * as path from 'path';
import { execSync } from 'child_process';

// tools/opencv-imgproc.js run as a subprocess, the way the server runs it:
// every output line is json, the last stdout line is the result event.
// Synthetic photos only. The model tests skip when models/ lacks the
// weights (see doc/HANDOFF-imgproc-models.md), the segment tests need the
// promptable model (mobilesam by default, about 1.5 s per photo; the big
// sam takes 8 s and 7.5 GB, its test only runs when its weights exist).

const root = import.meta.url.replace(/^file:\/\//, '').replace(/[^/]*\/[^/]*\/[^/]*$/, '');
const TOOL = path.join(root, 'tools', 'opencv-imgproc.js');
const MODELS = path.join(root, 'models');
const haveU2 = fs.existsSync(path.join(MODELS, 'u2netp.onnx'));
const have = (...f) => f.every(x => fs.existsSync(path.join(MODELS, x)));
const haveSam = have('mobile_sam_image_encoder.onnx', 'sam_mask_decoder_multi.onnx');
const haveBigSam = have('vision_encoder.onnx', 'prompt_encoder_mask_decoder.onnx');
const dir = path.join(process.env.TMPDIR ?? '/tmp', 'imgproc-test-' + process.pid);

fs.mkdirSync(dir, { recursive: true });

// tinytest compares arrays and objects by identity
const eq = (a, b) => strictEq(JSON.stringify(a), JSON.stringify(b));

let runs = 0;

// returns { code, lines (parsed stdout), result, err }
function run(args) {
  const base = path.join(dir, 'run' + runs++);

  execSync(`qjsm ${TOOL} ${args} > ${base}.out 2> ${base}.err; echo $? > ${base}.code`);

  const lines = fs.readFileSync(base + '.out', 'utf8').split('\n').filter(l => l.length).map(l => JSON.parse(l));

  return { code: Number(fs.readFileSync(base + '.code', 'utf8')), lines, result: lines[lines.length - 1], err: fs.readFileSync(base + '.err', 'utf8') };
}

// a bright disc with a textured face on a dark blue gradient
const DISC = { x: 320, y: 250, r: 130 };

function makePhoto(file) {
  const img = new cv.Mat(480, 640, cv.CV_8UC3, new cv.Scalar(0, 0, 0));

  for(let y = 0; y < 480; y++) cv.line(img, new cv.Point(0, y), new cv.Point(639, y), new cv.Scalar(70 + y / 12, 30 + y / 20, 20), 1);

  cv.circle(img, new cv.Point(DISC.x, DISC.y), DISC.r, new cv.Scalar(40, 200, 240), -1);
  cv.circle(img, new cv.Point(DISC.x - 40, DISC.y - 30), 30, new cv.Scalar(20, 60, 200), -1);
  cv.circle(img, new cv.Point(DISC.x + 45, DISC.y + 20), 22, new cv.Scalar(200, 90, 30), -1);
  cv.GaussianBlur(img, img, new cv.Size(3, 3), 0.7);
  cv.imwrite(file, img);

  return img;
}

const photo = path.join(dir, 'disc.jpg');

makePhoto(photo);

const alphaOf = file => cv.split(cv.imread(file, cv.IMREAD_UNCHANGED))[3];

function discIoU(alpha, offset = { x: 0, y: 0 }) {
  const truth = new cv.Mat(480, 640, cv.CV_8UC1, new cv.Scalar(0));

  cv.circle(truth, new cv.Point(DISC.x, DISC.y), DISC.r, new cv.Scalar(255), -1);

  const mine = new cv.Mat(480, 640, cv.CV_8UC1, new cv.Scalar(0));

  alpha.copyTo(mine.roi(new cv.Rect(offset.x, offset.y, alpha.cols, alpha.rows)));
  cv.threshold(mine, mine, 127, 255, cv.THRESH_BINARY);

  const both = new cv.Mat();
  const either = new cv.Mat();

  cv.bitwise_and(truth, mine, both);
  cv.bitwise_or(truth, mine, either);

  return cv.countNonZero(both) / cv.countNonZero(either);
}

tests({
  'help is one result event'() {
    const r = run('--help');

    eq(r.code, 0);
    eq(r.lines.length, 1);
    eq(r.result.event, 'result');
    assert(/cutout/.test(r.result.usage), 'usage text');
  },
  'every error is one ndjson line with its exit code'() {
    const cases = [
      ['frobnicate', 2, 'E_USAGE'],
      [`cutout --in ${photo}`, 2, 'E_USAGE'],
      [`cutout --in /nonexistent.jpg --out ${dir}/x.png`, 3, 'E_INPUT'],
      [`cutout --in ${photo} --out ${dir}/x.jpg`, 4, 'E_FORMAT'],
      [`cutout --in ${photo} --out ${dir}/x.png --method grabcut`, 9, 'E_METHOD'],
      [`cutout --in ${photo} --out ${dir}/x.png --dnn-model nope`, 8, 'E_MODEL'],
      [`segment --in ${photo} --out ${dir}/x.png`, 2, 'E_USAGE'],
      [`segment --in ${photo} --out ${dir}/x.png --point 5000,5`, 2, 'E_USAGE'],
      [`segment --in ${photo} --out ${dir}/x.png --point 5,5,7`, 2, 'E_USAGE'],
    ];

    for(const [args, code, name] of cases) {
      const r = run(args);

      eq(r.lines.length, 1);
      eq(r.code, code);
      eq(r.result.ok, false);
      eq(r.result.event, 'result');
      eq(r.result.error.code, name);
    }

    assert(!fs.existsSync(path.join(dir, 'x.jpg')) && !fs.existsSync(path.join(dir, 'x.png')), 'no output file after an error');
  },
  'a zero byte input is E_INPUT'() {
    const empty = path.join(dir, 'empty.jpg');

    fs.writeFileSync(empty, '');
    eq(run(`info --in ${empty}`).result.error.code, 'E_INPUT');
  },
  'info and a job file'() {
    const info = run(`info --in ${photo}`).result;

    eq([info.width, info.height, info.hasAlpha], [640, 480, false]);

    const job = path.join(dir, 'job.json');

    fs.writeFileSync(job, JSON.stringify({ in: photo }));
    eq(run(`info --job ${job}`).result.width, 640);
    eq(run(`info --job ${job} --in /nonexistent.jpg`).result.error.code, 'E_INPUT');
  },
  'capabilities lists the models and what loads'() {
    const r = run('capabilities').result;

    eq(r.ok, true);
    eq(r.methods, ['dnn']);
    eq(r.models.map(m => m.name), ['u2netp', 'sam', 'mobilesam']);
    eq(r.models[0].loaded, haveU2);
    assert(/^\d+\.\d+\.\d+/.test(r.opencv), 'opencv version');
  },
  'cutout: soft alpha, trimmed, right place'() {
    if(!haveU2) return print('  skipped: models/u2netp.onnx missing');

    const out = path.join(dir, 'cut.png');
    const r = run(`cutout --in ${photo} --out ${out} --trim --mask-out ${dir}/mask.png --outline-out ${dir}/outline.json`).result;

    eq(r.ok, true);
    eq(r.output.hasAlpha, true);
    eq(r.model.name, 'u2netp');

    const alpha = alphaOf(out);

    eq([alpha.cols, alpha.rows], [r.bbox.w, r.bbox.h]);
    assert(discIoU(alpha, r.offset) > 0.9, 'IoU with the disc ' + discIoU(alpha, r.offset));

    // a soft edge: some pixels are neither 0 nor 255
    const mid = new cv.Mat();

    cv.inRange(alpha, new cv.Scalar(10), new cv.Scalar(245), mid);
    assert(cv.countNonZero(mid) > 50, 'soft edge pixels');

    const outline = JSON.parse(fs.readFileSync(path.join(dir, 'outline.json'), 'utf8'));

    assert(outline[0].polygon.length >= 8, 'outline has points');
    eq(fs.existsSync(path.join(dir, 'mask.png')), true);
  },
  'cutout never leaves a temp file and is deterministic'() {
    if(!haveU2) return print('  skipped: models/u2netp.onnx missing');

    const a = path.join(dir, 'det_a.png');
    const b = path.join(dir, 'det_b.png');

    run(`cutout --in ${photo} --out ${a} --seed 3`);
    run(`cutout --in ${photo} --out ${b} --seed 3`);
    eq(fs.readFileSync(a).byteLength, fs.readFileSync(b).byteLength);
    eq(new Uint8Array(fs.readFileSync(a)).join(','), new Uint8Array(fs.readFileSync(b)).join(','));
    eq(fs.readdirSync(dir).filter(f => /\.tmp-/.test(f)).length, 0);
  },
  'cutout rest image: soft hole, or filled and opaque'() {
    if(!haveU2) return print('  skipped: models/u2netp.onnx missing');

    const rest = path.join(dir, 'rest.png');
    const filled = path.join(dir, 'rest_fill.png');

    run(`cutout --in ${photo} --out ${dir}/c.png --rest-out ${rest}`);
    run(`cutout --in ${photo} --out ${dir}/c.png --rest-out ${filled} --rest-fill inpaint`);

    const a = alphaOf(rest);
    const b = alphaOf(filled);

    eq(a.data[DISC.y * 640 + DISC.x], 0);
    eq(a.data[5], 255);
    eq(cv.countNonZero(b), 640 * 480);
  },
  '--progress: monotonic percent, named stages, result last'() {
    if(!haveU2) return print('  skipped: models/u2netp.onnx missing');

    const r = run(`cutout --in ${photo} --out ${dir}/p.png --progress`);
    const events = r.lines.slice(0, -1);

    eq(r.result.event, 'result');
    assert(events.length >= 8, 'several events: ' + events.length);

    let last = 0;

    for(const e of events) {
      eq(e.event, 'progress');
      assert(e.percent >= last, 'percent never goes back');
      assert(e.stage.length > 0 && e.message.length > 0, 'stage and message');
      last = e.percent;
    }

    eq(last, 100);
    eq(events[events.length - 1].stage, 'done');
    assert(new Set(events.map(e => e.stage)).size >= 6, 'distinct stages');
    eq(r.err, '');
  },
  'without --progress stdout is exactly one line'() {
    eq(run(`info --in ${photo}`).lines.length, 1);
  },
  'segment: taps, candidates, cache'() {
    if(!haveSam) return print('  skipped: sam models missing');

    const cache = path.join(dir, 'cache');
    const base = `segment --in ${photo} --cache-dir ${cache} --trim`;

    const miss = run(`prepare --in ${photo} --cache-dir ${cache}`).result;

    eq([miss.ok, miss.cache], [true, 'miss']);
    eq(run(`prepare --in ${photo} --cache-dir ${cache}`).result.cache, 'hit');

    // a tap on the disc returns the disc, on the background the background
    const on = run(`${base} --point ${DISC.x},${DISC.y} --out ${dir}/on.png --candidates 3`).result;

    eq([on.ok, on.cache], [true, 'hit']);
    eq(on.candidates.length, 3);
    assert(on.candidates.some(c => c.picked), 'one candidate is picked');
    assert(on.candidates.every(c => fs.existsSync(c.path)), 'every candidate has a file');
    assert(on.candidates.some(c => discIoU(alphaOf(c.path), c.bbox) > 0.9), 'a candidate is the disc');

    const off = run(`${base} --point 20,20 --out ${dir}/off.png`).result;

    assert(off.mask.coverage != on.mask.coverage, 'the background is not the disc');
    assert(off.bbox.w * off.bbox.h > on.bbox.w * on.bbox.h, 'the background is bigger than the disc');

    // normalised units give the same tap
    const norm = run(`${base} --point ${DISC.x / 640},${DISC.y / 480} --units norm --out ${dir}/norm.png`).result;

    eq(norm.bbox, run(`${base} --point ${DISC.x},${DISC.y} --out ${dir}/abs.png`).result.bbox);

    // a box picks the candidate that fits it
    const boxed = run(`${base} --box 170,100,300,300 --out ${dir}/box.png`).result;

    assert(boxed.boxIou > 0.5, 'box iou ' + boxed.boxIou);

    // --preview-size works on a smaller photo and reports the scale
    const small = run(`${base} --point ${DISC.x},${DISC.y} --preview-size 320 --out ${dir}/small.png`).result;

    eq(small.scale, 0.5);
    eq(small.original, { width: 640, height: 480 });
    assert(small.bbox.w <= 320, 'bbox is in preview pixels');
  },
  'segment with the big sam model'() {
    if(!haveBigSam) return print('  skipped: sam weights missing');

    const r = run(`segment --in ${photo} --model sam --cache-dir ${dir}/cache --point ${DISC.x},${DISC.y} --candidates 3 --trim --out ${dir}/big.png`).result;

    eq([r.ok, r.model.name], [true, 'sam']);
    assert(r.candidates.some(c => discIoU(alphaOf(c.path), c.bbox) > 0.9), 'a candidate is the disc');
  },
  '--phases is checked'() {
    const r = run(`segment --in ${photo} --out ${dir}/x.png --point 5,5 --phases final,preview`);

    eq([r.code, r.result.error.code, r.lines.length], [2, 'E_USAGE', 1]);
    eq(run(`cutout --in ${photo} --out ${dir}/x.png --phases both`).result.error.code, 'E_USAGE');
  },
  'segment --view-size: preview result, then final'() {
    if(!haveSam) return print('  skipped: sam models missing');

    const out = path.join(dir, 'ph.png');
    const r = run(`segment --in ${photo} --cache-dir ${dir}/cache --point ${DISC.x},${DISC.y} --view-size 320 --trim --out ${out}`);

    eq(r.lines.map(l => l.event + ':' + l.phase), ['result:preview', 'result:final']);

    const [pre, fin] = r.lines;

    eq([pre.ok, fin.ok, r.code], [true, true, 0]);
    eq([pre.scale, pre.view, pre.original], [0.5, { width: 320, height: 240 }, { width: 640, height: 480 }]);
    eq([fin.scale, fin.view], [1, { width: 640, height: 480 }]);
    eq([pre.output.path, fin.output.path], [path.join(dir, 'ph.preview.png'), out]);
    eq(fs.existsSync(pre.output.path) && fs.existsSync(out), true);

    // the same mask at both tiers: normalised boxes agree, pixel boxes scale
    for(const k of ['x', 'y', 'w', 'h']) assert(Math.abs(pre.bboxNorm[k] - fin.bboxNorm[k]) < 0.03, 'bboxNorm.' + k);

    assert(pre.bbox.w <= 320 && Math.abs(fin.bbox.w / pre.bbox.w - 2) < 0.2, 'bbox is in tier pixels, half the final one');
    assert(discIoU(alphaOf(out), fin.offset) > 0.9, 'the final mask is the disc');

    // every result carries the outline in the pixels of its own output
    for(const r of [pre, fin]) {
      eq(r.outlines.length, 1);
      assert(r.outlines[0].polygon.length >= 8, 'outline has points');
      assert(r.outlines[0].polygon.every(([x, y]) => x >= 0 && y >= 0 && x <= r.output.width && y <= r.output.height), 'outline inside the output');
    }
  },
  'segment --phases preview and --phases final alone'() {
    if(!haveSam) return print('  skipped: sam models missing');

    const base = `segment --in ${photo} --cache-dir ${dir}/cache --point ${DISC.x},${DISC.y}`;
    const a = run(`${base} --phases preview --view-size 320 --out ${dir}/only.png`);
    const b = run(`${base} --phases final --out ${dir}/only_f.png`);

    eq([a.lines.length, a.result.phase, b.lines.length, b.result.phase], [1, 'preview', 1, 'final']);
    eq([fs.existsSync(path.join(dir, 'only.preview.png')), fs.existsSync(path.join(dir, 'only.png'))], [true, false]);
  },
  'cutout --view-size: preview result, then final, rest only in final'() {
    if(!haveU2) return print('  skipped: models/u2netp.onnx missing');

    const out = path.join(dir, 'cp.png');
    const r = run(`cutout --in ${photo} --view-size 320 --trim --out ${out} --rest-out ${dir}/cp_rest.png --mask-out ${dir}/cp_mask.png`);

    eq(r.lines.map(l => l.phase), ['preview', 'final']);
    eq([r.lines[0].scale, r.lines[1].scale], [0.5, 1]);
    eq([fs.existsSync(path.join(dir, 'cp.preview.png')), fs.existsSync(path.join(dir, 'cp_mask.preview.png'))], [true, true]);
    eq([fs.existsSync(path.join(dir, 'cp_rest.preview.png')), fs.existsSync(path.join(dir, 'cp_rest.png'))], [false, true]);
    assert(discIoU(alphaOf(out), r.lines[1].offset) > 0.9, 'the final cut is the disc');
  },
  '--progress with phases: percent never goes back, phase named'() {
    if(!haveU2) return print('  skipped: models/u2netp.onnx missing');

    const r = run(`cutout --in ${photo} --view-size 320 --out ${dir}/pp.png --progress`);
    const prog = r.lines.filter(l => l.event == 'progress');
    let last = 0;

    for(const e of prog) {
      assert(e.percent >= last, 'percent never goes back');
      last = e.percent;
    }

    eq([prog[0].phase, prog[prog.length - 1].phase, last], ['preview', 'final', 100]);
    assert(prog.findIndex(e => e.phase == 'final') > prog.findIndex(e => e.phase == 'preview'), 'preview first');
    eq(r.result.phase, 'final');
  },
  'a missing model names the directory it looked in'() {
    const r = run(`cutout --in ${photo} --out ${dir}/nm.png --models-dir ${dir}/nowhere`);

    eq([r.code, r.result.error.code, r.result.error.detail.modelsDir], [8, 'E_MODEL', path.join(dir, 'nowhere')]);
  },
  '--png sets the compression level'() {
    if(!haveU2) return print('  skipped: models/u2netp.onnx missing');

    const a = path.join(dir, 'z0.png');
    const b = path.join(dir, 'z9.png');

    run(`cutout --in ${photo} --out ${a} --png 0`);
    run(`cutout --in ${photo} --out ${b} --png 9`);
    assert(fs.statSync(a).size > fs.statSync(b).size, 'level 0 is bigger');
  },
});

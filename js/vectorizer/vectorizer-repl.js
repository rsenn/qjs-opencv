// vectorizer-repl.js
//
// Interactive, non-GUI client for the vectorizer pipeline. Built as the
// go-to-sleep deliverable while the GUI's os.Worker-based stage 3
// (js/cvThreadPool.js + vector/poolWorker.js) has an unresolved hang - see
// vectorizer-debug.md. This talks to the exact same core/ Pipeline the GUI
// uses, but calls Pipeline.process() directly, in-process, synchronously -
// no worker thread, no SharedChannel, no postMessage. If this REPL vectorizes
// real photos reliably (it does - see BUGS), that's a second, independent
// confirmation that core/, cv/, and vector/methods/*.js were never the
// problem: the bug is isolated to the worker transport layer.
//
// Usage:
//   qjsm js/vectorizer/vectorizer-repl.js [path...]
//
// Any paths given on the command line are loaded immediately; then you're
// dropped into a REPL with the pipeline pre-wired as globals. Type help()
// for the command list. Because this is a real JS REPL (the same `repl`
// module cv-shell.js uses), you're never limited to the convenience
// functions below - `model`, `pipeline`, `registry`, `loader`, `composer`
// are all exposed directly too.

import * as fs from 'fs';
import * as path from 'path';
import * as std from 'std';
import { REPL } from 'repl';

import { Model } from './core/model.js';
import { Pipeline } from './core/pipeline.js';
import { Composer } from './core/composer.js';
import { defaultRegistry } from './vector/registry.js';
import { Loader } from './cv/loader.js';

const model = new Model();
const registry = defaultRegistry();
const loader = new Loader();
const composer = new Composer();
const pipeline = new Pipeline(model, { loader, registry, composer });

// The GUI's ProcessStage keeps in-progress (not-yet-run) param edits
// separate from model.results (which only holds the last *successful* run) -
// mirror that here rather than piggybacking on model.results, so set()
// before the first go() has somewhere to live.
const workingParams = new Map(); // frameId -> params object

let currentId = null;

function currentFrame() {
  if(!model.frames.length) return null;
  if(currentId && model.frame(currentId)) return model.frame(currentId);
  currentId = model.frames[0].id;
  return model.frame(currentId);
}

function resolveFrame(frameId) {
  return frameId != null ? model.frame(frameId) : currentFrame();
}

function fmtFrame(f) {
  const methodId = model.assignments.get(f.id);
  const method = methodId && registry.get(methodId);
  const res = model.results.get(f.id);
  let s = `${f.id}  ${f.label}  ${f.w}x${f.h}  method=${method ? method.id : '(none)'}`;
  if(res) s += `  last=${res.vectorData.shapes.length} shapes`;
  return s;
}

function load(...paths) {
  if(!paths.length) { console.log('usage: load(path, ...)  — file, or directory of images/video'); return; }
  const found = pipeline.discover(paths);
  if(!found.length) { console.log('no usable image/video files found in', paths); return; }
  for(const s of found) {
    const src = model.addSource(s.kind, s.uri, s.frameCount);
    let mat;
    try {
      mat = loader.extractFrame(src, 0);
    } catch(e) {
      console.log('load failed for', s.uri, ':', String((e && e.message) || e));
      continue;
    }
    const fr = model.addFrame(src.id, mat, path.basename(s.uri), mat.cols, mat.rows);
    if(!currentId) currentId = fr.id;
    console.log('loaded', fmtFrame(fr));
  }
}

function sources() {
  if(!model.sources.length) { console.log('(no sources — load() something)'); return; }
  model.sources.forEach(s => console.log(s.id, s.kind, s.uri, `frames=${s.frameCount}`));
}

function frames() {
  if(!model.frames.length) { console.log('(no frames — load() something)'); return; }
  model.frames.forEach(f => console.log((f.id === currentId ? '* ' : '  ') + fmtFrame(f)));
}

function frame(id) {
  if(id == null) return currentFrame();
  if(!model.frame(id)) { console.log('no such frame:', id, '— see frames()'); return; }
  currentId = id;
  return currentFrame();
}

function methods() {
  registry.list().forEach(m => console.log(m.id, '-', m.label));
}

function use(methodId, frameId) {
  const f = resolveFrame(frameId);
  if(!f) { console.log('no current frame — load() something first'); return; }
  const method = registry.get(methodId);
  if(!method) { console.log('unknown method:', methodId, '— see methods()'); return; }
  model.assign(f.id, methodId);
  workingParams.set(f.id, method.defaults());
  console.log('assigned', methodId, 'to', f.id);
  params(f.id);
}

function params(frameId) {
  const f = resolveFrame(frameId);
  if(!f) { console.log('no current frame'); return; }
  const methodId = model.assignments.get(f.id);
  const method = methodId && registry.get(methodId);
  if(!method) { console.log('frame has no assigned method — see use()'); return; }
  const p = workingParams.get(f.id) || method.defaults();
  workingParams.set(f.id, p);
  method.paramsSpec().forEach(spec => {
    const range = spec.min != null ? `, ${spec.min}..${spec.max}` : '';
    console.log(` ${spec.key} = ${p[spec.key]}  (${spec.type}${range})`);
  });
  return p;
}

function set(key, value, frameId) {
  const f = resolveFrame(frameId);
  if(!f) { console.log('no current frame'); return; }
  if(!model.assignments.has(f.id)) { console.log('frame has no assigned method — see use()'); return; }
  const p = workingParams.get(f.id) || {};
  p[key] = value;
  workingParams.set(f.id, p);
  console.log(key, '=', value);
}

function go(frameId) {
  const f = resolveFrame(frameId);
  if(!f) { console.log('no current frame — load() something first'); return; }
  const methodId = model.assignments.get(f.id);
  if(!methodId) { console.log('no method assigned — see use()'); return; }
  const p = workingParams.get(f.id) || registry.get(methodId).defaults();
  const t0 = Date.now();
  try {
    const vd = pipeline.process(f.id, p);
    console.log(`ok: ${vd.shapes.length} shapes in ${Date.now() - t0}ms`);
    return vd;
  } catch(e) {
    console.log(`error after ${Date.now() - t0}ms:`, String((e && e.stack) || e));
  }
}

function svg(outPath = 'out.svg') {
  if(!model.vectorizedFrames().length) { console.log('nothing vectorized yet — go() first'); return; }
  const out = pipeline.composeSVG();
  fs.writeFileSync(outPath, out);
  console.log('wrote', outPath, `(${out.length} bytes)`);
}

function quit(code = 0) {
  std.exit(code);
}

function help() {
  console.log(`
qjs-vectorizer REPL — no GUI, no worker threads, just the pipeline, direct.

  load(path, ...)          discover + load frame(s) (file, or dir of files)
  sources()                 list discovered sources
  frames()                  list loaded frames (* marks the current one)
  frame([id])               get, or switch, the current frame
  methods()                 list registered vectorize methods
  use(methodId[, frameId])  assign a method + reset its params to defaults
  params([frameId])         show current param values for the assigned method
  set(key, value[, id])     set one param on the current (or given) frame
  go([frameId])             run the assigned method now — synchronous, in-process
  svg([outPath])            compose + write SVG for every vectorized frame
  quit([code])              exit
  help()                    this message

Everything is also reachable directly: model, pipeline, registry, loader,
composer are plain globals, and this is a real JS REPL — any expression
works.
`);
}

Object.assign(globalThis, {
  model, pipeline, registry, loader, composer,
  load, sources, frames, frame, methods, use, params, set, go, svg, quit, help,
});

async function main(...args) {
  console.log('qjs-vectorizer REPL. Type help() for commands.');
  if(args.length) load(...args);

  const repl = new REPL('vectorizer');
  await repl.run();
}

main(...(globalThis['scriptArgs'] || []).slice(1));

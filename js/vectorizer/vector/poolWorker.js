// vector/poolWorker.js
//
// Worker entry point for running vectorize methods (vector/methods/*.js, via
// vector/registry.js) through js/cvThreadPool.js's generic ThreadPool, off
// the GUI thread. Kept in its own file rather than self-sourced via
// `new os.Worker(import.meta.url)` - see js/cvThreadPoolWorker.js's header
// comment and BUGS: qjsm-worker-self-reference-drops-messages.
//
// The onmessage handler below is FULLY SYNCHRONOUS, deliberately mirroring
// js/cvThreadPoolWorker.js.
//
// NOT CURRENTLY WIRED UP: stage3-process.js was switched to calling
// VectorMethod.apply() directly, in-process, after a hang was found where a
// job dispatched to a pooled worker was never observed arriving there (see
// vectorizer-debug.md). This file is left as-is, unused, for whoever
// root-causes that and re-enables the worker path.
//
// Protocol matches js/cvThreadPoolWorker.js exactly (parent -> worker:
// { id, method, args }; worker -> parent: { id, result } | { id, error }),
// except `method` here is a vector/registry.js method id (e.g. 'canny'),
// not a cv.* function name, and `args` is always [mat, params, meta] where
// meta is a plain { width, height } - looked up via defaultRegistry() and
// dispatched through VectorMethod.apply(mat, params, meta) instead of
// cv[method](...args). Unlike cvThreadPoolWorker.js there is no out-param
// Mat to echo back: vector/base.js's contract is "apply() must not mutate
// mat" and it returns VectorData (a plain, structured-clone-safe object
// tree - see core/vectordata.js).
import * as os from 'os';
import { SharedChannel } from '../../cvChannel.js';
import { defaultRegistry } from './registry.js';

console.log('[DIAG worker] booting'); // TEMP
const parent = os.Worker.parent;
const chan = new SharedChannel(parent);
const registry = defaultRegistry();
console.log('[DIAG worker] ready, methods:', registry.list().map((m) => m.id).join(',')); // TEMP

function noopTick() {}

chan.onmessage = (e) => {
  const { id, method: methodId, args } = e.data;
  const [mat, params, meta] = args;
  console.log(`[DIAG worker] onmessage id=${id} method=${methodId} mat=${mat && mat.cols}x${mat && mat.rows}`); // TEMP
  try {
    const method = registry.get(methodId);
    if (!method) throw new Error(`unknown vectorize method: ${methodId}`);
    const t0 = Date.now(); // TEMP
    const result = method.apply(mat, params, Object.assign({}, meta, { tick: noopTick }));
    console.log(`[DIAG worker] apply() done in ${Date.now() - t0}ms id=${id} shapes=${result && result.shapes && result.shapes.length}`); // TEMP
    chan.postMessage({ id, result });
    console.log(`[DIAG worker] postMessage sent id=${id}`); // TEMP
  } catch (err) {
    console.log(`[DIAG worker] apply() threw id=${id}:`, String((err && err.stack) || err)); // TEMP
    chan.postMessage({ id, error: String((err && err.message) || err) });
  }
};

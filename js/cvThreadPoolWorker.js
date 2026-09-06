// js/cvThreadPoolWorker.js
//
// Worker-thread entry for cvThreadPool.js. Kept in its own file rather than
// self-sourced via `new os.Worker(import.meta.url)`: confirmed (see BUGS,
// entry qjsm-worker-self-reference-drops-messages) that when the SAME file
// is loaded as both the main-thread module and the worker's entry module,
// worker -> parent postMessage traffic is silently dropped. Separate files
// avoid it entirely - this is the same reason tests/test_worker_body.js is
// split from tests/test_worker.js.
//
// Protocol (parent -> worker): { id, method, args }. Protocol (worker ->
// parent): { id, result, args } | { id, error }. SharedChannel marshals any
// Mat/TypedArray/DataView found in a message through a SharedArrayBuffer -
// but matToShared() (cvChannel.js) COPIES bytes into a freshly-allocated SAB
// each time a message crosses, it does not hand out a live alias of the
// original buffer. So an output Mat the caller passed in is NOT mutated by
// the worker calling it "in place" from the caller's point of view - the
// worker's `args` are reconstructed over their own fresh copies. To honor
// OpenCV's out-param convention anyway, this worker echoes back the whole
// (post-call, worker-side-mutated) `args` array; cvThreadPool.js copies each
// returned Mat's bytes into the caller's original Mat object.
import * as os from 'os';
import * as cv from 'opencv';
import { SharedChannel } from './cvChannel.js';

const parent = os.Worker.parent;
const chan = new SharedChannel(parent);

chan.onmessage = (e) => {
  const { id, method, args } = e.data;
  try {
    const fn = cv[method];
    if (typeof fn !== 'function') throw new Error(`cv.${method} is not a function`);
    const result = fn(...args);
    chan.postMessage({ id, result, args });
  } catch (err) {
    chan.postMessage({ id, error: String((err && err.message) || err) });
  }
};

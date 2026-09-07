// js/cvThreadPool.js
//
// A small pool of reusable os.Worker threads for offloading cv.* calls off
// the GUI thread. Call pool.run('MethodName', [args...]) - it dispatches to
// whichever pooled worker is free (queuing if all are busy) and resolves
// with the method's return value.
//
// Pass Mats created with cvChannel.js's newSharedMat() as out-params: a
// SharedArrayBuffer crossing an os.Worker boundary shares memory rather than
// copying it, so the worker mutates the exact same bytes the caller holds -
// no result needs to travel back at all. A plain (non-SAB) Mat can still be
// used as an out-param, but costs a copy: cvChannel.js's matToShared() has to
// snapshot it into a fresh SAB to send it, so the worker mutates its own
// reconstructed copy, and the copy-back below (copyMatInto) is what makes
// that copy visible on the caller's side again. Prefer SAB-backed Mats
// (newSharedMat) for anything performance-sensitive.
//
// Worker code lives in the sibling cvThreadPoolWorker.js file, not sourced
// from this same file via `new os.Worker(import.meta.url)` - see that
// file's header comment and BUGS: qjsm-worker-self-reference-drops-messages
// for why the self-referencing form silently drops worker -> parent
// messages on this engine.
//
// terminate() is currently a best-effort no-op: quickjs-libc.c's os.Worker
// (see TODO.md, "os.Worker termination (experimental, C-level, risky)")
// exposes no terminate() at all, and its finalizer only frees the JS-side
// pipe wrappers - the underlying OS thread runs forever regardless. Don't
// create/destroy pools repeatedly; make one for the process's lifetime.
//
// For a caller that reissues the same logical job as its input changes
// (a trackbar being dragged) use runLatest(key, method, args) instead of
// run(): a job still waiting in the queue when a newer call for the same
// key arrives is skipped for free, and a job already executing in a
// worker (which can't be stopped - see the TODO.md entry above) has its
// result silently discarded if it's no longer the latest by the time it
// finishes, so a stale result can never reach the caller.
//
// Example:
//   const pool = new ThreadPool();
//   const edges = new Mat();
//   await pool.run('Canny', [gray, edges, 50, 150]);   // edges filled in place
//   pool.terminate();

import * as os from 'os';
import { Mat } from 'opencv';
import { SharedChannel, isSharedMat } from './cvChannel.js';

const HERE = import.meta.url.replace(/^file:\/\//, '').replace(/[^/]+$/, '');
const WORKER_PATH = HERE + 'cvThreadPoolWorker.js';

// Copies src's pixel bytes into dst's existing backing buffer (same size -
// dst was already `.create()`d by the caller, or is a bare `new Mat()` that
// only became a real size once the worker's out-param call filled it in,
// in which case dst is resized to match).
function copyMatInto(dst, src) {
  if (dst.rows !== src.rows || dst.cols !== src.cols || dst.type() !== src.type()) {
    dst.create(src.rows, src.cols, src.type());
  }
  new Uint8Array(dst.buffer).set(new Uint8Array(src.buffer));
}

class PoolWorker {
  constructor(path) {
    this.chan = new SharedChannel(new os.Worker(path));
    this.busy = false;
    this._pending = new Map(); // id -> {resolve, reject, args}
    this.chan.onmessage = (e) => {
      const { id, result, args, error } = e.data;
      const p = this._pending.get(id);
      if (!p) return;
      this._pending.delete(id);
      this.busy = false;
      if (error != null) {
        p.reject(new Error(error));
        return;
      }
      // SAB-backed args already alias the worker's memory directly - nothing
      // to copy. A plain Mat's mutations only exist in the worker's own
      // reconstructed copy, so bring those back into the caller's Mat.
      for (let i = 0; i < p.args.length; i++) {
        if (p.args[i] instanceof Mat && args[i] instanceof Mat && !isSharedMat(p.args[i])) {
          copyMatInto(p.args[i], args[i]);
        }
      }
      p.resolve(result);
    };
  }

  run(id, method, args) {
    return new Promise((resolve, reject) => {
      this.busy = true;
      this._pending.set(id, { resolve, reject, args });
      this.chan.postMessage({ id, method, args });
    });
  }

  terminate() {
    this.chan.terminate();
  }
}

// Thrown by a runLatest() call that a newer call (same key) has superseded -
// either skipped before ever reaching a worker, or discarded after the
// worker finished. Callers that don't care why a run didn't produce a
// result can just swallow this specific error.
export class Superseded extends Error {
  constructor(key) {
    super(`superseded by a newer runLatest('${key}', ...) call`);
    this.key = key;
  }
}

export class ThreadPool {
  constructor(size = 4, { workerPath = WORKER_PATH } = {}) {
    this.size = size;
    this.workers = [];
    for (let i = 0; i < size; i++) this.workers.push(new PoolWorker(workerPath));
    this.queue = [];
    this._nextId = 1;
    this._latestGen = new Map(); // runLatest key -> generation counter
  }

  // Runs cv.<method>(...args) on a pooled worker; resolves with its return
  // value once done. Queues if every worker is currently busy.
  run(method, args = []) {
    return new Promise((resolve, reject) => {
      this.queue.push({ id: this._nextId++, method, args, resolve, reject });
      this._pump();
    });
  }

  // Like run(), but for a job that's about to be made obsolete by the next
  // call under the same `key` (e.g. "stage3:canny-contours" while a trackbar
  // is being dragged). Only the most recently issued call for a given key
  // ever resolves; every earlier one rejects with Superseded - either
  // immediately if it was still queued (skipped, never dispatched to a
  // worker), or once the worker's already-in-flight result comes back (the
  // worker itself can't be interrupted - see TODO.md).
  runLatest(key, method, args = []) {
    const gen = (this._latestGen.get(key) || 0) + 1;
    this._latestGen.set(key, gen);
    const isLatest = () => this._latestGen.get(key) === gen;
    return new Promise((resolve, reject) => {
      this.queue.push({ id: this._nextId++, method, args, key, gen, isLatest, resolve, reject });
      this._pump();
    });
  }

  // Dispatch several independent jobs and wait for all of them - the
  // "parallelizable step" of a pipeline. Order of results matches `jobs`.
  runAll(jobs) {
    return Promise.all(jobs.map(({ method, args }) => this.run(method, args)));
  }

  _pump() {
    while (this.queue.length) {
      const w = this.workers.find((w) => !w.busy);
      if (!w) break; // all busy - next _pump() call (on any job completion) retries
      const job = this.queue.shift();
      if (job.isLatest && !job.isLatest()) {
        // A newer runLatest() call for this key arrived while this one was
        // still queued - skip it for free, no worker time spent at all.
        job.reject(new Superseded(job.key));
        continue;
      }
      w.run(job.id, job.method, job.args)
        .then((result) => {
          if (job.isLatest && !job.isLatest()) throw new Superseded(job.key);
          return result;
        })
        .then(job.resolve, job.reject)
        .finally(() => this._pump());
    }
  }

  terminate() {
    for (const w of this.workers) w.terminate();
    this.workers.length = 0;
  }
}

// cv/jobs.js
//
// JobRunner: owns one os.Worker, sequences vectorization jobs, coalesces
// rapid-fire trackbar updates so at most one job is in flight + one queued.
// The GUI thread calls run(...) and gets progress/done/error callbacks; the
// Worker runs the actual algorithm.

import * as os from 'os';
import { matToMsg } from './marshal.js';

export class JobRunner {
  constructor(workerPath) {
    this.worker = new os.Worker(workerPath);
    this.worker.onmessage = (e) => this._onmessage(e.data);
    this._seq = 0;
    this._current = null;  // { id, cbs }
    this._pending = null;  // { id, methodId, frame, params, meta, cbs }
  }

  run(methodId, mat, params, meta, cbs = {}) {
    const id = ++this._seq;
    const job = { id, methodId, params, meta, cbs, frame: matToMsg(mat) };
    if (this._current) {
      if (this._pending && this._pending.cbs && this._pending.cbs.onCancel) this._pending.cbs.onCancel();
      this._pending = job;
    } else {
      this._post(job);
    }
    return id;
  }

  _post(job) {
    this._current = { id: job.id, cbs: job.cbs };
    this.worker.postMessage({
      type: 'run',
      id: job.id,
      methodId: job.methodId,
      params: job.params,
      meta: job.meta,
      frame: job.frame,
    });
  }

  _onmessage(msg) {
    const cur = this._current;
    if (!cur || msg.id !== cur.id) return;
    if (msg.type === 'progress') {
      if (cur.cbs.onProgress) cur.cbs.onProgress(msg.value);
      return;
    }
    if (msg.type === 'done') {
      if (cur.cbs.onDone) cur.cbs.onDone(msg.vd);
    } else if (msg.type === 'error') {
      if (cur.cbs.onError) cur.cbs.onError(msg.message);
    }
    this._current = null;
    if (this._pending) {
      const p = this._pending;
      this._pending = null;
      this._post(p);
    }
  }

  terminate() {
    if (this.worker.terminate) {
      try { this.worker.terminate(); } catch (_) {}
    }
  }
}

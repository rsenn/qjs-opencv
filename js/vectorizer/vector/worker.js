// vector/worker.js
//
// os.Worker entry point. Each algorithm runs here, off the GUI thread.
//
// Message protocol (parent -> worker):
//   { type:'run', id, methodId, params, meta:{width,height}, frame:{rows,cols,type,buffer} }
// Worker -> parent:
//   { id, type:'progress', value }       // 0..1
//   { id, type:'done',     vd }          // VectorData
//   { id, type:'error',    message }

import * as os from 'os';
import { defaultRegistry } from './registry.js';
import { msgToMat } from '../cv/marshal.js';

const parent = os.Worker.parent;
const registry = defaultRegistry();

parent.onmessage = (e) => {
  const msg = e.data;
  if (msg.type !== 'run') return;
  const { id, methodId, params, frame, meta } = msg;
  const post = (m) => parent.postMessage(Object.assign({ id }, m));
  const method = registry.get(methodId);
  if (!method) return post({ type: 'error', message: `unknown method: ${methodId}` });

  let mat = null;
  try {
    mat = msgToMat(frame);
    post({ type: 'progress', value: 0 });
    const onProgress = (t) => post({ type: 'progress', value: Math.max(0, Math.min(1, +t || 0)) });
    const vd = method.apply(mat, params, Object.assign({}, meta, { onProgress }));
    post({ type: 'progress', value: 1 });
    post({ type: 'done', vd });
  } catch (err) {
    post({ type: 'error', message: String((err && err.stack) || err) });
  } finally {
    if (mat && typeof mat.delete === 'function') {
      try { mat.delete(); } catch (_) {}
    }
  }
};

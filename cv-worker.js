// cv-worker.js — qjs-opencv RPC worker (runs inside an os.Worker thread).
//
// Accepts generic OpenCV calls over postMessage and shares result images back to
// the main thread via SharedArrayBuffer-backed Mats (zero pixel copy across the
// thread boundary — only the SAB header travels, the bytes are shared memory).
//
// Project convention: named imports, no `cv.` prefix, *Sync fs only.
// `ocv` is the ONE reflective handle, obtained via dynamic import purely so that
// "any opencv call" can be dispatched by name — the only thing a named import
// literally cannot express. Worker is an `os` primitive; no fs equivalent exists.

import { Mat } from 'opencv.so';
import * as os from 'os';

const parent = os.Worker.parent;

// Mat registry — Mats never cross the thread boundary, only their string ids do.
const mats = new Map();
let nextId = 1;
const put = m => {
  const h = 'm' + nextId++;
  mats.set(h, m);
  return h;
};

// Mat metadata. Flip ()/property style here if your build differs.
const info = m => ({
  rows: m.rows,
  cols: m.cols,
  type: m.type(),
  channels: m.channels(),
  elemSize: m.elemSize(),
});

import('opencv.so').then(ocv => { // Resolve one RPC argument into a real value. //   { __mat: 'm3' }              -> registry Mat //   { __new: 'Size', args:[..] } -> new Size(...)   (Point / Rect / Scalar too) //   anything else                -> passed through (numbers, strings, ...) const arg = a => { if(a && typeof a === 'object' && !Array.isArray(a)) { if ('__mat' in a) { const m = mats.get(a.__mat);
        if(!m) throw new Error('unknown Mat handle: ' + a.__mat);
        return m;
      }
      if('__new' in a) return new ocv[a.__new](...(a.args || []).map(arg));
    }
    return a;
  };

  // Encode a return value so it survives a structured-clone postMessage.
  const enc = r => {
    if(r instanceof Mat) return { __mat: put(r), info: info(r) };
    if(Array.isArray(r)) return r.map(enc);
    return r ?? null; // primitives; undefined -> null
  };

  const ops = {
    // Generic dispatch — covers "any opencv call allowed via RPC".
    call({ method, args = [] }) {
      const fn = ocv[method];
      if(typeof fn !== 'function') throw new Error('no such opencv function: ' + method);
      return enc(fn(...args.map(arg)));
    },

    // Empty Mat — OpenCV allocates/sizes it on first write (cvtColor, blur, ...).
    newMat() {
      return { handle: put(new Mat()), info: null };
    },

    // Mat whose pixels live in a SharedArrayBuffer; hand the SAB back so the main
    // thread sees every subsequent write with no serialization.
    createSharedMat({ rows, cols, type }) {
      const probe = new Mat(rows, cols, type); // learn element size
      const bytes = rows * cols * probe.elemSize(); // contiguous, no row padding
      const sab = new SharedArrayBuffer(bytes);
      const m = new Mat(rows, cols, type, sab); // header over shared memory
      return { handle: put(m), sab, info: info(m) };
    },

    matInfo({ handle }) {
      return info(mats.get(handle));
    },

    release({ handle }) {
      return mats.delete(handle);
    },
  };

  parent.onmessage = e => {
    const { id, op, payload } = e.data;
    if(op === 'bye') {
      parent.onmessage = null;
      return;
    } // let the loop drain
    try {
      parent.postMessage({ id, ok: true, result: ops[op](payload) });
    } catch(err) {
      parent.postMessage({ id, ok: false, error: String((err && err.message) || err) });
    }
  };
});

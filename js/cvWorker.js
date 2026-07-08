// Serialize cv.Mat, TypedArray and DataView through an os.Worker boundary via
// SharedArrayBuffer. Structured clone of a SAB is shared (not copied), so both
// threads see the same live memory; this module handles the format info you
// need on the other side to reconstruct the view.
//
// Encoded shapes (all structured-clone-safe):
//   Mat        -> { __sab:'mat', rows, cols, type, step, sab }
//   TypedArray -> { __sab:'ta',  ctor, byteOffset, length, sab }
//   DataView   -> { __sab:'dv',  byteOffset, byteLength, sab }
//
// Any other value passes through as-is.

import { Mat } from 'opencv';

const TAG = '__sab';

// ---------- primitive ----------

// Copy bytes from a Uint8Array view into a fresh SharedArrayBuffer.
export function copyToShared(u8) {
  const sab = new SharedArrayBuffer(u8.byteLength);
  new Uint8Array(sab).set(u8);
  return sab;
}

// ---------- cv.Mat ----------

export function matToShared(mat) {
  const src = new Uint8Array(mat.buffer);
  const sab = copyToShared(src);
  return {
    [TAG]: 'mat',
    rows: mat.rows,
    cols: mat.cols,
    type: mat.type,
    step: mat.step,
    sab,
  };
}

export function sharedToMat(d) {
  return new Mat(d.rows, d.cols, d.type, d.sab, d.step);
}

// Allocate a fresh SAB and a Mat that aliases it. Returns { mat, desc } where
// `desc` can be posted through a SharedChannel; the receiving side reconstructs
// a Mat over the SAME memory (true zero-copy).
export function newSharedMat(rows, cols, type) {
  const probe = new Mat(1, 1, type);
  const elemBytes = probe.elemSize;
  const step = cols * elemBytes;
  const sab = new SharedArrayBuffer(rows * step);
  const mat = new Mat(rows, cols, type, sab, step);
  const desc = { [TAG]: 'mat', rows, cols, type, step, sab };
  return { mat, desc };
}

// ---------- TypedArray ----------

const TA_CTORS = {
  Int8Array,
  Uint8Array,
  Uint8ClampedArray,
  Int16Array,
  Uint16Array,
  Int32Array,
  Uint32Array,
  Float32Array,
  Float64Array,
};
if(typeof BigInt64Array !== 'undefined')
  TA_CTORS.BigInt64Array = BigInt64Array;
if(typeof BigUint64Array !== 'undefined')
  TA_CTORS.BigUint64Array = BigUint64Array;

export function typedArrayToShared(ta) {
  const src = new Uint8Array(ta.buffer, ta.byteOffset, ta.byteLength);
  const sab = copyToShared(src);
  return {
    [TAG]: 'ta',
    ctor: ta.constructor.name,
    byteOffset: 0,
    length: ta.length,
    sab,
  };
}

export function sharedToTypedArray(d) {
  const Ctor = TA_CTORS[d.ctor];
  if(!Ctor) throw new TypeError(`unknown TypedArray ctor: ${d.ctor}`);
  return new Ctor(d.sab, d.byteOffset, d.length);
}

// Fresh SAB + TypedArray view; both threads alias the same memory.
export function newSharedTypedArray(Ctor, length) {
  const sab = new SharedArrayBuffer(length * Ctor.BYTES_PER_ELEMENT);
  const ta = new Ctor(sab, 0, length);
  const desc = { [TAG]: 'ta', ctor: Ctor.name, byteOffset: 0, length, sab };
  return { ta, desc };
}

// ---------- DataView ----------

export function dataViewToShared(dv) {
  const src = new Uint8Array(dv.buffer, dv.byteOffset, dv.byteLength);
  const sab = copyToShared(src);
  return { [TAG]: 'dv', byteOffset: 0, byteLength: dv.byteLength, sab };
}

export function sharedToDataView(d) {
  return new DataView(d.sab, d.byteOffset, d.byteLength);
}

// ---------- deep encode / decode ----------

function isTypedArray(v) {
  return ArrayBuffer.isView(v) && !(v instanceof DataView);
}

// Walk value; replace Mat / TypedArray / DataView with descriptors. Plain
// objects and arrays are shallow-cloned so the caller's payload is untouched.
export function encode(value) {
  // qjs reports typeof mat === 'function', so the Mat check must run before
  // the primitive short-circuit.
  if(value instanceof Mat) return matToShared(value);
  if(value === null || typeof value !== 'object') return value;
  if(value instanceof DataView) return dataViewToShared(value);
  if(isTypedArray(value)) return typedArrayToShared(value);
  if(Array.isArray(value)) return value.map(encode);
  const out = {};
  for(const k of Object.keys(value)) out[k] = encode(value[k]);
  return out;
}

// Inverse of encode: reconstruct Mat / TypedArray / DataView from descriptors.
export function decode(value) {
  if(value === null || typeof value !== 'object') return value;
  if(TAG in value) {
    switch (value[TAG]) {
      case 'mat':
        return sharedToMat(value);
      case 'ta':
        return sharedToTypedArray(value);
      case 'dv':
        return sharedToDataView(value);
      default:
        throw new TypeError(`unknown ${TAG} tag: ${value[TAG]}`);
    }
  }
  if(Array.isArray(value)) return value.map(decode);
  const out = {};
  for(const k of Object.keys(value)) out[k] = decode(value[k]);
  return out;
}

// ---------- adapter ----------

// Wraps an os.Worker (parent side) or os.Worker.parent (child side). Same
// postMessage / onmessage shape; Mat / TypedArray / DataView anywhere in the
// payload are transparently marshalled through SharedArrayBuffer.
export class SharedChannel {
  constructor(endpoint) {
    this._ep = endpoint;
    this._handler = null;
    endpoint.onmessage = e => {
      if(!this._handler) return;
      this._handler({ data: decode(e.data) });
    };
  }
  postMessage(msg) {
    this._ep.postMessage(encode(msg));
  }
  set onmessage(fn) {
    this._handler = fn;
  }
  get onmessage() {
    return this._handler;
  }
  terminate() {
    if(this._ep.terminate) this._ep.terminate();
  }
}

// gui/trackbars.js
//
// Maps a method's declarative paramsSpec() to HighGUI trackbars. Trackbars are
// integer-only and cannot be individually removed, so when the parameter set
// changes we rebuild the window (see App.recreateWindow). We poll positions
// each frame instead of relying on callback semantics, which differ between
// builds — robust and simple.

import { createTrackbar, getTrackbarPos } from 'opencv.so';

export class Trackbars {
  constructor(windowName) { this.win = windowName; this.specs = []; }

  reset(windowName) { this.win = windowName; this.specs = []; }

  // spec: { key, type, min, max, step, default, options? }
  add(spec, current) {
    const count = spec.type === 'bool' ? 1
      : spec.type === 'enum' ? (spec.options.length - 1)
      : Math.max(1, Math.round((spec.max - spec.min) / (spec.step || 1)));
    const pos = this._valueToPos(spec, current ?? spec.default);
    try { createTrackbar(spec.label, this.win, pos, count, () => {}); } catch (_) {}
    this.specs.push({ spec, last: pos });
  }

  _valueToPos(spec, v) {
    if (spec.type === 'bool') return v ? 1 : 0;
    if (spec.type === 'enum') return v | 0;
    return Math.round((v - spec.min) / (spec.step || 1));
  }

  _posToValue(spec, pos) {
    if (spec.type === 'bool') return pos ? 1 : 0;
    if (spec.type === 'enum') return pos | 0;
    if (spec.type === 'int') return Math.round(spec.min + pos * (spec.step || 1));
    return +(spec.min + pos * (spec.step || 1)).toFixed(4);
  }

  // Returns a params delta object if anything changed, else null.
  poll() {
    let changed = null;
    for (const s of this.specs) {
      let pos = s.last;
      try { pos = getTrackbarPos(s.spec.label, this.win); } catch (_) {}
      if (pos !== s.last) {
        s.last = pos;
        (changed || (changed = {}))[s.spec.key] = this._posToValue(s.spec, pos);
      }
    }
    return changed;
  }
}

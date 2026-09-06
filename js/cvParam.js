/*
 * cvParam.js
 *
 * Tunable pipeline parameters, ported from ../param.js (plot-cv) and trimmed
 * to what cvPipeline's dirty-flag recompute needs: a value holder that knows
 * whether it has changed since a processor last consumed it. No EventEmitter
 * dependency (cvPipeline.js reads `.dirty` directly instead of subscribing).
 */

import { isFunction } from './cvUtils.js';

const MinMax = (min, max) => value => Math.max(min, Math.min(max, value));

export class Param {
  dirty = true; /* starts dirty so the owning processor's first run always happens */

  valueOf() {
    return this.get();
  }

  toString() {
    return '' + this.valueOf();
  }

  createTrackbar(name, win, cv) {
    cv.createTrackbar(name, win + '', this.value, this.max, value => this.set(value));
  }
}

export class NumericParam extends Param {
  constructor(value = 0, min = 0, max = 1, step = 1) {
    super();
    const clamp = MinMax(min, max);
    this.value = clamp(value);
    Object.assign(this, { default: value, min, max, step, clamp });
  }

  get() {
    return this.value;
  }

  set(value) {
    const newValue = this.clamp(this.min + roundStep(value - this.min, this.step));
    if(newValue !== this.value) {
      this.value = newValue;
      this.dirty = true;
    }
    return this;
  }

  get alpha() {
    const { value, min, max } = this;
    return (value - min) / (max - min);
  }

  set alpha(a) {
    this.set(this.min + roundStep((this.max - this.min) * a, this.step));
  }

  reset() {
    this.set(this.default);
  }
}

export class EnumParam extends NumericParam {
  constructor(values, init = 0) {
    super(typeof init == 'number' ? init : values.indexOf(init), 0, values.length - 1);
    this.values = values;
  }

  get() {
    return this.values[Math.floor(NumericParam.prototype.get.call(this))];
  }

  set(newVal) {
    const i = typeof newVal == 'number' ? newVal : this.values.indexOf(newVal);
    if(i == -1) throw new Error(`No such value '${newVal}' in [${this.values}]`);
    return super.set(i);
  }
}

export class BoolParam extends NumericParam {
  constructor(value = false) {
    super(value ? 1 : 0, 0, 1, 1);
  }

  get() {
    return !!NumericParam.prototype.get.call(this);
  }

  set(value) {
    return super.set(value ? 1 : 0);
  }
}

function roundStep(v, step) {
  return step ? Math.round(v / step) * step : v;
}

/* True if any of the given params is dirty (a processor's `isDirty` getter
 * composes this with its own manual `.dirty` flag - see cvPipeline.js). */
export function anyDirty(params) {
  return params.some(p => (isFunction(p?.get) ? p.dirty : false));
}

/* Clear dirty on every given param (called once a processor that watched
 * them has actually run). */
export function clearDirty(params) {
  for(const p of params) if(p && 'dirty' in p) p.dirty = false;
}

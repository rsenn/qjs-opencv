// core/events.js
//
// Minimal synchronous observer. The Model emits; Views subscribe. Keeps the
// MVC wiring explicit without pulling in any framework.

export class Emitter {
  constructor() { this._h = new Map(); }

  on(type, fn) {
    if (!this._h.has(type)) this._h.set(type, new Set());
    this._h.get(type).add(fn);
    return () => this._h.get(type).delete(fn);
  }

  emit(type, payload) {
    const set = this._h.get(type);
    if (set) for (const fn of [...set]) fn(payload);
    const any = this._h.get('*');
    if (any) for (const fn of [...any]) fn({ type, payload });
  }
}

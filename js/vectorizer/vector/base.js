// vector/base.js
//
// The plugin contract (Strategy pattern). Every vectorization method is a class
// extending VectorMethod. A method declares:
//   - id / label / description           : identity for the registry + GUI
//   - paramsSpec()                        : declarative parameter schema, which
//                                           the stage-3 View turns into widgets
//   - async apply(mat, params, meta)      : the only place OpenCV runs; returns
//                                           VectorData (see core/vectordata.js)
//
// Because apply() returns the generic VectorData, the GUI, the projector and the
// SVG composer never know which method produced it. New methods are added purely
// by dropping a file in vector/methods/ and registering it — no other file
// changes. That is the "pluggable, separate class file" requirement.
//
// A ParamSpec entry:
//   { key, label, type:'int'|'float'|'bool'|'enum', min, max, step, default,
//     options?:[{value,label}] }
// The GUI maps int/float to trackbars (float via a fixed scale), bool to a
// switch and enum to radio buttons. Keeping it declarative means stage 3 is
// 100% generic across methods.
//
// apply() is async and must `await meta.tick(progress)` (progress 0..1)
// between named stages (grayscale -> blur -> edges -> contours, etc), not
// just call it. `tick()` also yields the event loop (an `os.sleepAsync(0)`
// under the hood), which is what lets the GUI keep redrawing/responding
// while a vectorize runs on the main thread instead of freezing for its
// whole duration - see BUGS: os.Worker is broken in the installed qjsm, so
// this cooperative-yield scheme is the actual concurrency model here, not
// a real background thread.

export class VectorMethod {
  static id = 'base';
  static label = 'Base';
  static description = '';

  get id() {
    return this.constructor['id'];
  }
  get label() {
    return this.constructor['label'];
  }
  get description() {
    return this.constructor['description'];
  }

  // Override: declarative parameter schema.
  paramsSpec() {
    return [];
  }

  // Convenience: default param object derived from the spec.
  defaults() {
    const o = {};
    for(const p of this.paramsSpec()) o[p.key] = p.default;
    return o;
  }

  // Override: produce VectorData from a cv.Mat. Must not mutate `mat`.
  // eslint-disable-next-line no-unused-vars
  async apply(mat, params, meta) {
    throw new Error(`${this.id}.apply() not implemented`);
  }
}

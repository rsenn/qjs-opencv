// vector/base.js
//
// The plugin contract (Strategy pattern). Every vectorization method is a class
// extending VectorMethod. A method declares:
//   - id / label / description           : identity for the registry + GUI
//   - paramsSpec()                        : declarative parameter schema, which
//                                           the stage-3 View turns into trackbars
//   - apply(mat, params, meta)            : the only place OpenCV runs; returns
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
// The GUI maps int/float to trackbars (float via a fixed scale), bool/enum to
// trackbars with 0..1 / 0..N ranges. Keeping it declarative means stage 3 is
// 100% generic across methods.

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
  apply(mat, params, meta) {
    throw new Error(`${this.id}.apply() not implemented`);
  }
}

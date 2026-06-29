// core/pipeline.js
//
// The generic four-stage pipeline: LOAD -> ASSIGN -> PROCESS -> PROJECT.
// It is deliberately abstract. It knows the *shape* of each stage's job but
// delegates the actual work to injected collaborators, so none of the OpenCV
// vectorization logic leaks in here. This is what makes the pipeline reusable
// for, say, an SVG-from-PDF tool instead of SVG-from-image.
//
//   loader   : { discover(uris) -> sources, extractFrame(source, index) -> frameHandle }
//   registry : vector-method registry (see vector/registry.js)
//   composer : { compose(model) -> svgString }
//
// Each stage validates that its prerequisites from the previous stage exist,
// so the GUI can grey-out "Next" until a stage is satisfiable.

import { Stage } from './model.js';

export class Pipeline {
  constructor(model, { loader, registry, composer }) {
    this.model = model;
    this.loader = loader;
    this.registry = registry;
    this.composer = composer;
  }

  // ---- Stage 1: LOAD -------------------------------------------------------
  discover(uris) { return this.loader.discover(uris); }

  // ---- Stage 2: ASSIGN -----------------------------------------------------
  // A default assignment so the user always has something to preview.
  autoAssign(defaultMethodId) {
    for (const f of this.model.frames)
      if (!this.model.assignments.has(f.id))
        this.model.assign(f.id, defaultMethodId);
  }

  // ---- Stage 3: PROCESS ----------------------------------------------------
  // Run the assigned method for one frame with given params -> VectorData.
  process(frameId, params) {
    const frame = this.model.frame(frameId);
    const methodId = this.model.assignments.get(frameId);
    if (!frame || !methodId) return null;
    const method = this.registry.get(methodId);
    const merged = Object.assign(method.defaults(), params || {});
    const vd = method.apply(frame.mat, merged, { width: frame.w, height: frame.h });
    this.model.setResult(frameId, merged, vd);
    return vd;
  }

  // ---- Stage 4: PROJECT ----------------------------------------------------
  composeSVG() { return this.composer.compose(this.model); }

  // ---- Stage gating --------------------------------------------------------
  canAdvance(fromStage) {
    const m = this.model;
    switch (fromStage) {
      case Stage.LOAD:    return m.frames.length > 0;
      case Stage.ASSIGN:  return m.frames.every((f) => m.assignments.has(f.id));
      case Stage.PROCESS: return m.vectorizedFrames().length > 0;
      case Stage.PROJECT: return true;
      default:            return false;
    }
  }
}

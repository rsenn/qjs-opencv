// gui/stages/stage3-process.js
//
// Stage 3 — PROCESS. Pick a frame (prev/next), see its assigned method, and
// tune that method's parameters via trackbars built from method.paramsSpec().
// A split preview shows the source on the left and the rasterized VectorData on
// the right, so the result matches the exported SVG (raster.js reuses the same
// flatten()/geometry the SVG composer uses).
//
// Parameter editing is fully generic: this stage never names a concrete method
// or parameter. It reads paramsSpec(), drives the Trackbars manager, and calls
// pipeline.process(frameId, params). That is the decoupling the brief asked for.

import { Mat, Size, Scalar, drawRect, Rect, FILLED } from 'opencv.so';
import { Palette } from '../canvas.js';
import { drawVectorData } from '../../cv/raster.js';

const FILL = typeof FILLED === 'number' ? FILLED : -1;

export class ProcessStage {
  constructor() { this.idx = 0; this.preview = null; }

  _frame(app) {
    const frames = app.model.frames;
    if (!frames.length) return null;
    this.idx = Math.max(0, Math.min(this.idx, frames.length - 1));
    return frames[this.idx];
  }

  enter(app) {
    const f = this._frame(app);
    if (!f) return;
    const method = app.registry.get(app.model.assignments.get(f.id));
    if (!method) return;

    // Build trackbars from the declarative spec. Seed from any prior result.
    const prior = app.model.results.get(f.id);
    const params = prior ? prior.params : method.defaults();
    for (const spec of method.paramsSpec()) app.trackbars.add(spec, params[spec.key]);

    this._run(app, f, params);
  }

  // A trackbar moved: collect the full current param set and re-run. We read
  // every trackbar's current value (not just the delta) so apply() always gets
  // a complete, self-consistent params object.
  onParams(app, _delta) {
    const f = this._frame(app);
    if (!f) return;
    const method = app.registry.get(app.model.assignments.get(f.id));
    const params = method.defaults();
    for (const s of app.trackbars.specs)
      params[s.spec.key] = app.trackbars._posToValue(s.spec, s.last);
    this._run(app, f, params);
  }

  _run(app, frame, params) {
    try {
      const vd = app.pipeline.process(frame.id, params);
      this._renderPreview(app, frame, vd);
    } catch (e) {
      console.log('vectorize error:', String(e && e.message || e));
      this.preview = null;
    }
  }

  // Rasterize VectorData onto a white Mat sized to the frame, centered on its
  // content, for the right-hand preview pane.
  _renderPreview(app, frame, vd) {
    if (!vd) { this.preview = null; return; }
    const w = frame.w || 640, h = frame.h || 480;
    const mat = new Mat(new Size(w, h), 16 /* CV_8UC3 */);
    drawRect(mat, new Rect(0, 0, w, h), new Scalar(250, 250, 250), FILL);
    drawVectorData(mat, vd, null, { strokeScale: 1 });
    this.preview = { mat, count: vd.shapes.length };
  }

  draw(app) {
    const cv = app.canvas, hud = app.hud, m = app.model;
    const f = this._frame(app);
    if (!f) { cv.text('No frames to process.', 16, 80, Palette.danger, 0.5); return; }
    const method = app.registry.get(m.assignments.get(f.id));

    // header + frame nav
    cv.text(`Tune parameters  ·  ${f.label}  ·  method: ${method ? method.label : '?'}`,
      16, 66, Palette.textDim, 0.46);
    const frames = m.frames;
    if (hud.button(cv, app.W - 220, 56, 30, 26, '<')) this._step(app, -1);
    cv.text(`${this.idx + 1}/${frames.length}`, app.W - 184, 74, Palette.text, 0.45);
    if (hud.button(cv, app.W - 150, 56, 30, 26, '>')) this._step(app, 1);

    // split preview: source (left) | vector preview (right)
    const top = 92, ph = app.H - top - 28, halfW = (app.W - 48) / 2;
    cv.panel(16, top, halfW, ph, Palette.panel);
    cv.text('source', 24, top + 18, Palette.textDim, 0.4);
    if (f.mat) cv.paste(f.mat, 22, top + 24, halfW - 12, ph - 36);

    const rx = 32 + halfW;
    cv.panel(rx, top, halfW, ph, Palette.panel);
    cv.text(this.preview ? `vectorized · ${this.preview.count} shapes` : 'vectorizing…',
      rx + 8, top + 18, Palette.textDim, 0.4);
    if (this.preview) cv.paste(this.preview.mat, rx + 6, top + 24, halfW - 12, ph - 36);

    cv.text('Drag the trackbars under the window title to adjust the method.',
      16, app.H - 8, Palette.textDim, 0.4);
  }

  _step(app, d) {
    this.idx += d;
    app.goTo(app.model.stage);  // re-enter to rebuild trackbars for the new frame
  }
}

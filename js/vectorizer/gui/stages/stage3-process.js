/*
 * gui/stages/stage3-process.js
 *
 * Stage 3 - PROCESS. Pick a frame (prev/next), see its assigned method, and
 * tune that method's parameters via cvWidgets trackbar widgets built from
 * method.paramsSpec(). A split preview shows the source on the left and the
 * rasterized VectorData on the right, so the result matches the exported SVG
 * (raster.js reuses the same flatten()/geometry the SVG composer uses).
 *
 * Parameter editing is fully generic: this stage never names a concrete
 * method or parameter. It reads paramsSpec(), draws one trackbar per entry,
 * and calls pipeline.process(frameId, params). Unlike the native HighGUI
 * trackbars this replaced, the drawn ones need no window rebuild when the
 * param set changes (e.g. stepping to a frame with a different method) -
 * see cvWidgets.js's Hud.trackbar().
 */

import { Mat, Size, Scalar, rectangle, Rect, FILLED, CV_8UC3 } from 'opencv';
import { Palette } from '../canvas.js';
import { drawVectorData } from '../../cv/raster.js';

const FILL = typeof FILLED === 'number' ? FILLED : -1;

/*
 * A vectorize is a potentially expensive call (see _run()) - re-running it
 * on every single trackbar-drag frame or wheel tick would make
 * dragging/scrolling feel like wading through mud. Instead:
 *   - dragging a trackbar re-runs at most once every DRAG_PACE_MS, then
 *     once more on release to make sure the exact final value lands;
 *   - wheel steps (discrete, often fired in a fast burst) wait for
 *     WHEEL_IDLE_MS of no further wheel activity before re-running once.
 */
const DRAG_PACE_MS = 150;
const WHEEL_IDLE_MS = 200;

export class ProcessStage {
  constructor() {
    this.idx = 0;
    this.preview = null;
    this.busy = false; /* a vectorize call is in flight (coarse - see _run()) */
    this.lastError = null; /* last error message, if any */
    this.params = null; /* current param values, drawn as trackbar widgets */
    this._dirty = false; /* params changed since the last _run() */
    this._lastChangeAt = 0; /* Date.now() of the most recent param edit */
    this._lastRunAt = 0; /* Date.now() of the most recent _run() */
    this._wasDragging = false; /* hud.dragKey state as of the previous frame */
    this.view = { zoom: 1, panX: 0, panY: 0 }; /* shared pan/zoom for both preview panels */
    this._panDrag = null; /* {x,y} last mouse pos while panning, else null */
  }

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

    /* Seed params from any prior result, else the method's declared defaults.
     * No trackbar registration needed - draw() reads method.paramsSpec()
     * fresh every frame and draws whatever is current. */
    const prior = app.model.results.get(f.id);
    this.params = prior ? { ...prior.params } : method.defaults();

    this._dirty = false;
    this._wasDragging = false;
    // Fresh frame - reset to native resolution, top-left. Per-frame view state
    // (not shared across frames) since frames can differ in size.
    this.view = { zoom: 1, panX: 0, panY: 0 };
    this._panDrag = null;
    this._run(app, f, this.params);
  }

  // Runs the method directly, in-process, on the GUI thread - a deliberate
  // pivot away from the os.Worker-based app.vectorPool (js/cvThreadPool.js +
  // vector/poolWorker.js), which had an unresolved hang where a dispatched
  // job's message was never observed arriving at the worker (see
  // vectorizer-debug.md). vectorizer-repl.js exercises this exact same
  // direct Pipeline.process() call path and resolves reliably, which is why
  // it's the fallback here too - see BUGS. This blocks app.run()'s frame
  // loop for the call's duration, but with cv/loader.js's load-time
  // downscale to MAX_DIM already in place, that's under a second for most
  // methods on real photos; a persistently slow method is a follow-up
  // problem to solve once the app is reliable again, not before.
  //
  // No supersession/race guard is needed any more: since apply() runs
  // synchronously to completion before this function returns, there is
  // never a second _run() in flight to race against - unlike the async
  // runLatest()-based version this replaced, frame can't change mid-call.
  _run(app, frame, params) {
    const frameId = frame.id;
    const methodId = app.model.assignments.get(frameId);
    const method = app.registry.get(methodId);
    if (!method) return;
    const merged = Object.assign(method.defaults(), params || {});
    this.busy = true;
    this.lastError = null;
    try {
      const vd = method.apply(frame.mat, merged, { width: frame.w, height: frame.h, tick: () => {} });
      app.model.setResult(frameId, merged, vd);
      this.busy = false;
      this._renderPreview(app, frame, vd);
    } catch (e) {
      this.busy = false;
      this.lastError = String((e && e.message) || e);
      console.log('vectorize error:', this.lastError);
    }
  }

  // Rasterize VectorData onto a white Mat sized to the frame, centered on its
  // content, for the right-hand preview pane.
  _renderPreview(app, frame, vd) {
    if (!vd) { this.preview = null; return; }
    const w = frame.w || 640, h = frame.h || 480;
    const mat = new Mat(new Size(w, h), CV_8UC3);
    rectangle(mat, new Rect(0, 0, w, h), new Scalar(250, 250, 250), FILL);
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

    // split preview: source (left) | vector preview (right). Reserve room
    // at the bottom for the drawn trackbars (one row per paramsSpec() entry)
    // so they never overlap the preview panels.
    const specs = method ? method.paramsSpec() : [];
    const barsH = specs.length ? specs.length * 26 + 8 : 20;
    const top = 92, ph = app.H - top - barsH, halfW = (app.W - 48) / 2;
    cv.panel(16, top, halfW, ph, Palette.panel);
    cv.text(`source  ·  ${Math.round(this.view.zoom * 100)}%  ·  drag to pan, wheel to zoom`,
      24, top + 18, Palette.textDim, 0.4);
    const leftRect = { x: 22, y: top + 24, w: halfW - 12, h: ph - 36 };

    const rx = 32 + halfW;
    cv.panel(rx, top, halfW, ph, Palette.panel);
    const status = this.lastError
      ? `error: ${this.lastError.split('\n')[0]}`
      : this.busy
        ? 'vectorizing…'
        : this.preview ? `vectorized · ${this.preview.count} shapes` : 'idle';
    cv.text(status, rx + 8, top + 18, this.lastError ? Palette.danger : Palette.textDim, 0.4);
    const rightRect = { x: rx + 6, y: top + 24, w: halfW - 12, h: ph - 36 };

    if (f.mat) {
      this._updateViewport(app, leftRect, rightRect, f.w || f.mat.cols, f.h || f.mat.rows);
      cv.pasteViewport(f.mat, leftRect.x, leftRect.y, leftRect.w, leftRect.h, this.view);
    }
    if (this.preview) cv.pasteViewport(this.preview.mat, rightRect.x, rightRect.y, rightRect.w, rightRect.h, this.view);

    // busy indicator (coarse - _run() is synchronous, so this only ever
    // renders on the frame right after a call already finished) across the
    // bottom of the preview pane
    if (this.busy) {
      const by = top + ph - 14, bx = rx + 6, bw = halfW - 12;
      cv.rect(bx, by, bw, 6, Palette.panel2, true);
      cv.rect(bx, by, bw, 6, Palette.accent || Palette.text, true);
    }

    // one drawn trackbar per paramsSpec() entry - no window rebuild needed
    // when the spec set changes (e.g. stepping to a frame with a different
    // method), unlike the native HighGUI trackbars this replaced.
    if (specs.length && this.params) {
      const barsTop = app.H - barsH + 4;
      specs.forEach((spec, i) => {
        const y = barsTop + i * 26;
        const row = [16, y, app.W - 32, 24, spec, this.params[spec.key]];
        const newValue = spec.type === 'bool' ? hud.switchControl(cv, ...row)
          : spec.type === 'enum' ? hud.radioGroup(cv, ...row)
          : hud.trackbar(cv, ...row);
        if (newValue != null) {
          this.params[spec.key] = newValue; // cheap: updates the drawn value/label immediately
          this._dirty = true;
          this._lastChangeAt = Date.now();
        }
      });
      this._maybeRun(app, f);
    } else {
      cv.text('This method has no parameters.', 16, app.H - 8, Palette.textDim, 0.4);
    }
  }

  // Cursor-centered wheel zoom + drag-to-pan, shared across the source and
  // preview panels (they're the same frame size, so one view keeps them in
  // sync for comparison). Either panel can start a drag or receive a wheel
  // event; the resulting view applies to both.
  _updateViewport(app, leftRect, rightRect, imgW, imgH) {
    const inRect = (mx, my, r) => mx >= r.x && mx < r.x + r.w && my >= r.y && my < r.y + r.h;
    const mouse = app.mouse, v = this.view;
    const hitRect = inRect(mouse.x, mouse.y, leftRect) ? leftRect
      : inRect(mouse.x, mouse.y, rightRect) ? rightRect : null;

    if (app.wheel && hitRect) {
      const sx = mouse.x - hitRect.x, sy = mouse.y - hitRect.y;
      const imgX = v.panX + sx / v.zoom, imgY = v.panY + sy / v.zoom;
      // GDK reports scroll-up as a negative delta (see icvHighGUIonMouse in
      // window_gtk.cpp) - scroll up/away zooms in, matching image viewers.
      const factor = app.wheel.delta < 0 ? 1.15 : 1 / 1.15;
      v.zoom = Math.max(0.05, Math.min(8, v.zoom * factor));
      v.panX = imgX - sx / v.zoom;
      v.panY = imgY - sy / v.zoom;
    }

    if (mouse.down) {
      if (!this._panDrag && hitRect) this._panDrag = { x: mouse.x, y: mouse.y };
      else if (this._panDrag) {
        v.panX -= (mouse.x - this._panDrag.x) / v.zoom;
        v.panY -= (mouse.y - this._panDrag.y) / v.zoom;
        this._panDrag = { x: mouse.x, y: mouse.y };
      }
    } else {
      this._panDrag = null;
    }

    const visW = Math.max(1, leftRect.w) / v.zoom, visH = Math.max(1, leftRect.h) / v.zoom;
    v.panX = Math.max(0, Math.min(Math.max(0, imgW - visW), v.panX));
    v.panY = Math.max(0, Math.min(Math.max(0, imgH - visH), v.panY));
  }

  // Decides whether this frame is the moment to actually pay for a
  // vectorize (see the pacing comment above the class). Never starts one
  // while another is still running - _run() awaits the pool call, so
  // without this.busy gating, a fast trackbar drag could fire overlapping
  // runs (runLatest() keeps only the newest one, but there is no reason to
  // even queue the earlier ones).
  _maybeRun(app, frame) {
    if (!this._dirty || this.busy) {
      this._wasDragging = app.hud.dragKey != null;
      return;
    }
    const now = Date.now();
    const dragging = app.hud.dragKey != null;
    const dragReleased = this._wasDragging && !dragging;
    const pacedWhileDragging = dragging && now - this._lastRunAt >= DRAG_PACE_MS;
    const settledAfterWheel = !dragging && now - this._lastChangeAt >= WHEEL_IDLE_MS;

    if (dragReleased || pacedWhileDragging || settledAfterWheel) {
      this._dirty = false;
      this._lastRunAt = now;
      this._run(app, frame, this.params);
    }
    this._wasDragging = dragging;
  }

  _step(app, d) {
    const frames = app.model.frames;
    if (!frames.length) return;
    this.idx = Math.max(0, Math.min(this.idx + d, frames.length - 1));
    this.enter(app); // reload params for the new frame's method, no window rebuild
  }
}

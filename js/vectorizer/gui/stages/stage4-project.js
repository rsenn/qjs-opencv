// gui/stages/stage4-project.js
//
// Stage 4 — PROJECT. The output canvas is shown scaled-to-fit. Every vectorized
// frame gets a placement: a 4-corner quad in canvas space. Dragging a corner
// recomputes the homography H = rectToQuad(frameW, frameH, quad) and stores it
// on the Model (model.setPlacement). The composite is previewed by rasterizing
// each frame's VectorData through its H — the exact transform the SVG composer
// bakes/emits, so preview == export.
//
// All projective math lives in core/geometry.js (pure JS). This stage only maps
// between screen pixels and canvas coordinates and drives drag interaction; it
// uses OpenCV solely to raster the preview.

import { Mat, Size, Scalar, Rect, drawRect, FILLED } from 'opencv.so';
import { Palette } from '../canvas.js';
import { drawVectorData } from '../../cv/raster.js';
import { rectToQuad } from '../../core/geometry.js';

const FILL = typeof FILLED === 'number' ? FILLED : -1;
const HANDLE = 7;          // corner handle radius in screen px
const COLORS = [[96, 168, 96], [70, 130, 200], [70, 170, 220], [180, 120, 90], [150, 90, 180], [90, 160, 160]];

export class ProjectStage {
  constructor() {
    this.drag = null;          // { frameId, corner }
    this.focused = null;
    this.view = { x: 0, y: 0, s: 1 };  // canvas->screen mapping
  }

  enter(app) {
    const m = app.model;
    // Seed default placements: cascade frames across the canvas so none overlap
    // perfectly, each at ~45% of canvas size.
    const vfs = m.vectorizedFrames();
    vfs.forEach((f, i) => {
      if (m.placements.has(f.id) && m.placements.get(f.id).quad) return;
      const cw = m.canvas.width, ch = m.canvas.height;
      const w = cw * 0.45, h = ch * 0.45;
      const ox = (cw - w) * (0.15 + 0.12 * (i % 4));
      const oy = (ch - h) * (0.15 + 0.12 * ((i / 4) | 0));
      const quad = [[ox, oy], [ox + w, oy], [ox + w, oy + h], [ox, oy + h]];
      m.setPlacement(f.id, { quad, H: rectToQuad(f.w || w, f.h || h, quad), opacity: 1 });
    });
    if (vfs.length && !vfs.some((f) => f.id === this.focused)) this.focused = vfs[0].id;

    // Opacity trackbar for the focused frame.
    const pl = this.focused && m.placements.get(this.focused);
    app.trackbars.add(
      { key: 'opacity', label: 'opacity %', type: 'int', min: 0, max: 100, step: 1, default: 100 },
      Math.round(((pl && pl.opacity) ?? 1) * 100),
    );
  }

  onParams(app, delta) {
    if (delta.opacity != null && this.focused)
      app.model.setPlacement(this.focused, { opacity: delta.opacity / 100 });
  }

  // ---- coordinate mapping --------------------------------------------------
  _fit(app) {
    const m = app.model, top = 92, pad = 16;
    const aw = app.W - 2 * pad, ah = app.H - top - 28;
    const s = Math.min(aw / m.canvas.width, ah / m.canvas.height);
    const dw = m.canvas.width * s, dh = m.canvas.height * s;
    this.view = { x: pad + (aw - dw) / 2, y: top + (ah - dh) / 2, s };
    return this.view;
  }
  _toScreen(p) { return [this.view.x + p[0] * this.view.s, this.view.y + p[1] * this.view.s]; }
  _toCanvas(x, y) { return [(x - this.view.x) / this.view.s, (y - this.view.y) / this.view.s]; }

  draw(app) {
    const cv = app.canvas, hud = app.hud, m = app.model;
    cv.text('Place & warp each frame onto the output  ·  drag the corner handles',
      16, 66, Palette.textDim, 0.46);

    const vfs = m.vectorizedFrames();
    if (!vfs.length) { cv.text('Nothing vectorized yet — go back to stage 3.', 16, 90, Palette.danger, 0.5); return; }

    const v = this._fit(app);

    // ---- composite raster preview -----------------------------------------
    // Render the whole canvas (in canvas resolution) then paste scaled. This is
    // the same path the SVG composer takes, just rasterized.
    const cM = new Mat(new Size(m.canvas.width, m.canvas.height), 16);
    drawRect(cM, new Rect(0, 0, m.canvas.width, m.canvas.height), new Scalar(250, 250, 250), FILL);
    for (const f of vfs) {
      const pl = m.placements.get(f.id);
      const res = m.results.get(f.id);
      if (!pl || !res || !pl.H) continue;
      drawVectorData(cM, res.vectorData, pl.H, { strokeScale: 1 });
    }
    cv.panel(v.x - 6, v.y - 6, m.canvas.width * v.s + 12, m.canvas.height * v.s + 12, Palette.panel);
    cv.paste(cM, v.x, v.y, m.canvas.width * v.s, m.canvas.height * v.s);
    cv.rect(v.x, v.y, m.canvas.width * v.s, m.canvas.height * v.s, Palette.line, false, 1);

    // ---- handle drag interaction ------------------------------------------
    this._handleDrag(app, vfs);

    // ---- draw quads + handles ---------------------------------------------
    vfs.forEach((f, i) => {
      const pl = m.placements.get(f.id);
      if (!pl || !pl.quad) return;
      const focused = f.id === this.focused;
      const col = COLORS[i % COLORS.length];
      const scr = pl.quad.map((p) => this._toScreen(p));
      for (let k = 0; k < 4; k++) {
        const a = scr[k], b = scr[(k + 1) % 4];
        cv.line(a[0], a[1], b[0], b[1], focused ? col : Palette.line, focused ? 2 : 1);
      }
      for (const c of scr)
        cv.rect(c[0] - HANDLE, c[1] - HANDLE, HANDLE * 2, HANDLE * 2,
          focused ? col : Palette.panel2, true);
      // label near first corner
      cv.text(f.label, scr[0][0] + 8, scr[0][1] - 8, focused ? Palette.text : Palette.textDim, 0.4);
    });

    // ---- frame selector strip ---------------------------------------------
    let bx = 16;
    vfs.forEach((f) => {
      const w = 120;
      if (hud.button(cv, bx, app.H - 26, w, 22, f.label.slice(0, 16),
        { active: f.id === this.focused, scale: 0.4 })) {
        this.focused = f.id;
        app.goTo(app.model.stage);   // rebuild opacity trackbar for new focus
      }
      bx += w + 6;
    });
  }

  _handleDrag(app, vfs) {
    const m = app.model, mouse = app.mouse;

    // Begin drag: on a fresh press, hit-test corner handles of any frame
    // (focused frame's handles win ties since drawn last / checked first).
    if (mouse.down && !this.drag) {
      const order = vfs.slice().sort((a, b) => (a.id === this.focused ? 1 : 0) - (b.id === this.focused ? 1 : 0));
      for (const f of order) {
        const pl = m.placements.get(f.id);
        if (!pl || !pl.quad) continue;
        for (let k = 0; k < 4; k++) {
          const s = this._toScreen(pl.quad[k]);
          if (Math.abs(mouse.x - s[0]) <= HANDLE + 2 && Math.abs(mouse.y - s[1]) <= HANDLE + 2) {
            this.drag = { frameId: f.id, corner: k };
            this.focused = f.id;
          }
        }
      }
    }

    // Continue drag: move the corner, recompute H, store on model.
    if (this.drag && mouse.down) {
      const f = m.frame(this.drag.frameId);
      const pl = m.placements.get(this.drag.frameId);
      if (f && pl && pl.quad) {
        const c = this._toCanvas(mouse.x, mouse.y);
        const quad = pl.quad.map((p) => p.slice());
        quad[this.drag.corner] = [
          Math.max(0, Math.min(m.canvas.width, c[0])),
          Math.max(0, Math.min(m.canvas.height, c[1])),
        ];
        m.setPlacement(this.drag.frameId, { quad, H: rectToQuad(f.w, f.h, quad) });
      }
    }

    // End drag.
    if (!mouse.down && this.drag) this.drag = null;
  }
}

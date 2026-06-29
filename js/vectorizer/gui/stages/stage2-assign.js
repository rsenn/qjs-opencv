// gui/stages/stage2-assign.js
//
// Stage 2 — ASSIGN. Two columns: on the left the selected frames (with their
// currently-assigned method), on the right the registry of vectorization
// methods. Click a frame to focus it, click a method to assign it to the
// focused frame. "Apply to all" assigns the focused method to every frame.
//
// This stage is pure View+Controller over the Model: it only ever calls
// model.assign() / reads model.assignments. It never touches OpenCV.

import { Palette } from '../canvas.js';

export class AssignStage {
  constructor() { this.focusedFrame = null; this.focusedMethod = null; }

  enter(app) {
    // Default focus: first frame, and that frame's current method.
    const frames = app.model.frames;
    if (frames.length && !frames.some((f) => f.id === this.focusedFrame))
      this.focusedFrame = frames[0].id;
    this.focusedMethod = app.model.assignments.get(this.focusedFrame) || null;
  }

  draw(app) {
    const cv = app.canvas, hud = app.hud, m = app.model, reg = app.registry;
    cv.text('Assign a vectorization method to each frame  ·  click a frame, then a method',
      16, 66, Palette.textDim, 0.46);

    // ---- left column: selected frames -------------------------------------
    const lx = 16, lw = 360, top = 84, rh = 64;
    cv.panel(lx, top, lw, app.H - top - 16, Palette.panel);
    cv.text('Frames', lx + 10, top + 22, Palette.text, 0.5);

    let y = top + 34;
    for (const f of m.frames) {
      const mid = m.assignments.get(f.id);
      const method = mid && reg.get(mid);
      const sel = f.id === this.focusedFrame;
      cv.rect(lx + 8, y, lw - 16, rh - 8, sel ? Palette.accent2 : Palette.panel2, true);
      if (f.mat) cv.paste(f.mat, lx + 12, y + 4, 64, rh - 16);
      cv.text(f.label, lx + 84, y + 22, sel ? Palette.bg : Palette.text, 0.44);
      cv.text(method ? method.label : '— unassigned —', lx + 84, y + 42,
        sel ? Palette.bg : (method ? Palette.textDim : Palette.danger), 0.4);
      if (hud._hit(lx + 8, y, lw - 16, rh - 8)) {
        this.focusedFrame = f.id;
        this.focusedMethod = mid || null;
      }
      y += rh;
    }

    // ---- right column: method palette -------------------------------------
    const rx = lx + lw + 20, rw = app.W - rx - 16;
    cv.panel(rx, top, rw, app.H - top - 16, Palette.panel);
    cv.text('Methods', rx + 10, top + 22, Palette.text, 0.5);

    const list = reg.list();
    const mh = 40;
    let my = top + 34;
    for (const method of list) {
      const assignedHere = this.focusedFrame && m.assignments.get(this.focusedFrame) === method.id;
      const focused = method.id === this.focusedMethod;
      cv.rect(rx + 8, my, rw - 16, mh - 6,
        assignedHere ? Palette.accent : (focused ? Palette.panel2 : Palette.panel), true);
      cv.rect(rx + 8, my, rw - 16, mh - 6, Palette.line, false, 1);
      cv.text(method.label, rx + 16, my + 17, assignedHere ? Palette.bg : Palette.text, 0.46);
      cv.text(method.description, rx + 16, my + 31, assignedHere ? Palette.bg : Palette.textDim, 0.36);
      if (hud._hit(rx + 8, my, rw - 16, mh - 6)) {
        this.focusedMethod = method.id;
        if (this.focusedFrame) m.assign(this.focusedFrame, method.id);
      }
      my += mh;
    }

    // ---- apply-to-all ------------------------------------------------------
    if (this.focusedMethod) {
      const by = app.H - 52;
      if (hud.button(cv, rx + 8, by, 200, 32,
        `Apply "${reg.get(this.focusedMethod).label}" to all`, { active: true }))
        for (const f of m.frames) m.assign(f.id, this.focusedMethod);
    }

    const assigned = m.frames.filter((f) => m.assignments.has(f.id)).length;
    cv.text(`${assigned}/${m.frames.length} frame(s) assigned`, lx, app.H - 4, Palette.textDim, 0.42);
  }
}

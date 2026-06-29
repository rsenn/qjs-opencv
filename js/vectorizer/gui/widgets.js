// gui/widgets.js
//
// Immediate-mode widgets. Each call draws a control into the Canvas AND reports
// whether a click landed on it this frame (clicks were collected from the mouse
// queue before draw). No retained widget tree: the View is a pure function of
// the Model, which keeps the MVC loop simple.

import { Palette } from './canvas.js';

export class Hud {
  constructor() { this.reset(); }

  reset() {
    this.clicks = [];     // [{x,y}] left-button-downs this frame
    this.pointer = { x: 0, y: 0 };
    this.isDown = false;
  }

  // Feed the per-frame input snapshot before drawing.
  frame(clicks, pointer, isDown) {
    this.clicks = clicks.slice();
    this.pointer = pointer;
    this.isDown = isDown;
  }

  _hit(x, y, w, h) {
    for (let i = 0; i < this.clicks.length; i++) {
      const c = this.clicks[i];
      if (c.x >= x && c.x <= x + w && c.y >= y && c.y <= y + h) {
        this.clicks.splice(i, 1);   // consume so only one widget reacts
        return true;
      }
    }
    return false;
  }

  hover(x, y, w, h) {
    const p = this.pointer;
    return p.x >= x && p.x <= x + w && p.y >= y && p.y <= y + h;
  }

  button(cv, x, y, w, h, label, opts = {}) {
    const hovered = this.hover(x, y, w, h);
    const base = opts.active ? Palette.accent : (hovered ? Palette.panel2 : Palette.panel);
    cv.rect(x, y, w, h, opts.color || base, true);
    cv.rect(x, y, w, h, Palette.line, false, 1);
    const scale = opts.scale || 0.5;
    const ts = cv.textSize(label, scale);
    cv.text(label, x + (w - ts.w) / 2, y + (h + ts.h) / 2,
      opts.active ? Palette.bg : Palette.text, scale);
    return !opts.disabled && this._hit(x, y, w, h);
  }

  toggle(cv, x, y, w, h, label, on) {
    const clicked = this.button(cv, x, y, w, h, (on ? '[x] ' : '[ ] ') + label,
      { active: !!on, scale: 0.45 });
    return clicked;
  }

  // A selectable row (used in lists). Returns true on click.
  row(cv, x, y, w, h, label, selected) {
    const hovered = this.hover(x, y, w, h);
    cv.rect(x, y, w, h, selected ? Palette.accent2 : (hovered ? Palette.panel2 : Palette.panel), true);
    cv.text(label, x + 8, y + h * 0.66, selected ? Palette.bg : Palette.text, 0.45);
    return this._hit(x, y, w, h);
  }

  // A thumbnail tile with caption + selection ring. The image itself is pasted
  // by the caller (needs the cv.Mat); this draws the frame + caption + handles
  // the click. Returns true on click.
  tile(cv, x, y, w, h, caption, selected) {
    cv.rect(x - 2, y - 2, w + 4, h + 18, selected ? Palette.accent : Palette.panel2, false, 2);
    cv.rect(x, y + h, w, 16, Palette.panel, true);
    cv.text(caption, x + 4, y + h + 12, Palette.textDim, 0.38);
    return this._hit(x - 2, y - 2, w + 4, h + 18);
  }
}

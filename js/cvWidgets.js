/*
 * cvWidgets.js
 *
 * Immediate-mode GUI widgets (button, toggle, row, tile, trackbar) shared by
 * any qjs-opencv app, migrated out of js/vectorizer/gui/widgets.js so they
 * aren't vectorizer-specific. Motivating case: HighGUI's native
 * cv.createTrackbar() can't be removed or have its range/count changed once
 * created in a window - the only way to change a trackbar set is to destroy
 * and recreate the whole window (see js/vectorizer/gui/trackbars.js, now
 * retired). The `trackbar()` widget here is drawn every frame like any other
 * widget, so switching which parameters are shown never touches the window.
 *
 * Two drawing backends behind one small Surface interface (rect/line/circle/
 * text/textSize), so widgets never know which is active:
 *   - CvSurface: plain cv.* pixel drawing into a Mat. Always available, no
 *     new C++ needed - this is the fallback and the only backend that has
 *     been exercised against a running vectorizer GUI so far.
 *   - NvgSurface: anti-aliased drawing via qjs-nanovg, for an HQ mode. Needs
 *     a GL context already current (e.g. a cv.namedWindow(win, WINDOW_OPENGL)
 *     + cv.setOpenGlContext(win), see js_opengl.cpp's setOpenGlContext/
 *     setOpenGlDrawCallback/updateWindow) - callers opt in explicitly via
 *     createHqSurface(); nothing here creates a GL context itself.
 *
 * createHqSurface() does the dynamic `import('nanovg')` and returns null if
 * the module isn't installed, so a caller can do:
 *   const nvg = await createHqSurface(w, h);
 *   const surface = nvg ?? new CvSurface(canvasMat);
 */

import { Mat, Size, Point, Rect, Scalar, rectangle, line, circle, putText, getTextSize, FONT_HERSHEY_SIMPLEX, FILLED } from 'opencv';

const FILL = typeof FILLED === 'number' ? FILLED : -1;

export const Palette = {
  bg: [28, 26, 24],
  panel: [44, 41, 38],
  panel2: [58, 54, 50],
  accent: [96, 168, 96] /* BGR -> a muted green */,
  accent2: [70, 130, 200] /* BGR -> warm blue */,
  text: [232, 230, 228],
  textDim: [150, 148, 146],
  line: [80, 76, 72],
  danger: [60, 60, 200],
};

function sc(c) {
  return new Scalar(c[0], c[1], c[2]);
}

/* bgr -> [r,g,b,a] for nanovg, which (like the rest of OpenGL/the web) is
 * RGB-ordered while every Palette entry here follows this codebase's BGR
 * convention (cv.Scalar order). */
function bgrToRgba(c, a = 255) {
  return [c[2], c[1], c[0], a];
}

/* ------------------------------------------------------------------ *
 * CvSurface: draws into a cv.Mat with plain cv.* pixel primitives.
 * ------------------------------------------------------------------ */
export class CvSurface {
  constructor(mat) {
    this.mat = mat;
  }

  get width() {
    return this.mat.cols ?? this.mat.width;
  }
  get height() {
    return this.mat.rows ?? this.mat.height;
  }

  rect(x, y, w, h, color, fill = true, thickness = 1) {
    rectangle(this.mat, new Rect(x | 0, y | 0, w | 0, h | 0), sc(color), fill ? FILL : thickness);
  }

  /* Rounded-ish panel (plain rect + lighter top edge for a bit of depth). */
  panel(x, y, w, h, color = Palette.panel) {
    this.rect(x, y, w, h, color, true);
    this.line(x, y, x + w, y, Palette.line, 1);
  }

  line(x1, y1, x2, y2, color = Palette.line, thickness = 1) {
    line(this.mat, new Point(x1 | 0, y1 | 0), new Point(x2 | 0, y2 | 0), sc(color), thickness);
  }

  circle(cx, cy, r, color, fill = true) {
    circle(this.mat, new Point(cx | 0, cy | 0), Math.max(1, r | 0), sc(color), fill ? FILL : 1);
  }

  text(str, x, y, color = Palette.text, scale = 0.5, thickness = 1) {
    putText(this.mat, String(str), new Point(x | 0, y | 0), FONT_HERSHEY_SIMPLEX, scale, sc(color), thickness);
  }

  textSize(str, scale = 0.5, thickness = 1) {
    try {
      const r = getTextSize(String(str), FONT_HERSHEY_SIMPLEX, scale, thickness);
      if(r && typeof r.width == 'number') return { w: r.width, h: r.height };
      if(Array.isArray(r)) {
        const [w, h] = r;
        return { w, h };
      }
    } catch(_) {}
    return { w: String(str).length * 8 * scale * 2, h: 12 };
  }
}

/* ------------------------------------------------------------------ *
 * NvgSurface: same interface, drawn with nanovg for anti-aliased "HQ" UI.
 * Caller owns the nanovg Context lifecycle (create/attach a GL context,
 * BeginFrame/EndFrame around a whole app frame) - this only issues the
 * per-widget draw calls in between.
 * ------------------------------------------------------------------ */
export class NvgSurface {
  constructor(nanovg, ctx, width, height) {
    this.nanovg = nanovg;
    this.ctx = ctx;
    this._width = width;
    this._height = height;
  }

  get width() {
    return this._width;
  }
  get height() {
    return this._height;
  }

  rect(x, y, w, h, color, fill = true, thickness = 1) {
    const { ctx } = this;
    ctx.BeginPath();
    ctx.Rect(x, y, w, h);
    if(fill) {
      ctx.FillColor(this.nanovg.RGBA(...bgrToRgba(color)));
      ctx.Fill();
    } else {
      ctx.StrokeWidth(thickness);
      ctx.StrokeColor(this.nanovg.RGBA(...bgrToRgba(color)));
      ctx.Stroke();
    }
  }

  panel(x, y, w, h, color = Palette.panel) {
    this.rect(x, y, w, h, color, true);
    this.line(x, y, x + w, y, Palette.line, 1);
  }

  line(x1, y1, x2, y2, color = Palette.line, thickness = 1) {
    const { ctx } = this;
    ctx.BeginPath();
    ctx.MoveTo(x1, y1);
    ctx.LineTo(x2, y2);
    ctx.StrokeWidth(thickness);
    ctx.StrokeColor(this.nanovg.RGBA(...bgrToRgba(color)));
    ctx.Stroke();
  }

  circle(cx, cy, r, color, fill = true) {
    const { ctx } = this;
    ctx.BeginPath();
    ctx.Circle(cx, cy, r);
    if(fill) {
      ctx.FillColor(this.nanovg.RGBA(...bgrToRgba(color)));
      ctx.Fill();
    } else {
      ctx.StrokeColor(this.nanovg.RGBA(...bgrToRgba(color)));
      ctx.Stroke();
    }
  }

  text(str, x, y, color = Palette.text, scale = 0.5) {
    const { ctx } = this;
    ctx.FontSize(Math.max(8, Math.round(scale * 32)));
    ctx.FillColor(this.nanovg.RGBA(...bgrToRgba(color)));
    ctx.Text(x, y, String(str));
  }

  textSize(str, scale = 0.5) {
    const { ctx } = this;
    ctx.FontSize(Math.max(8, Math.round(scale * 32)));
    const { width, height } = ctx.TextBounds2(0, 0, String(str));
    return { w: width, h: height };
  }
}

/*
 * Attempts the dynamic `import('nanovg')`; returns null (never throws) when
 * the module isn't installed, so callers can fall back to CvSurface without
 * a try/catch of their own. Does NOT create a GL context - the target
 * window must already be a WINDOW_OPENGL window with setOpenGlContext(name)
 * called, per js_opengl.cpp.
 */
export async function createHqSurface(width, height, flags = 0) {
  let nanovg;
  try {
    nanovg = await import('nanovg');
  } catch(_) {
    return null;
  }
  const ctx = nanovg.CreateGL3(flags || nanovg.ANTIALIAS | nanovg.STENCIL_STROKES);
  if(!ctx) return null;
  return new NvgSurface(nanovg, ctx, width, height);
}

/* ------------------------------------------------------------------ *
 * Hud: click/hover/drag hit-testing plus the widgets themselves. Backend-
 * agnostic - every widget takes a Surface (CvSurface or NvgSurface) as its
 * first argument and never touches cv or nanovg directly.
 * ------------------------------------------------------------------ */
export class Hud {
  constructor() {
    this.reset();
  }

  reset() {
    this.clicks = []; /* [{x,y}] left-button-downs this frame */
    this.pointer = { x: 0, y: 0 };
    this.isDown = false;
    this.dragKey = null; /* id of the trackbar currently being dragged, if any */
    this.wheel = null; /* {delta} from a mouse-wheel event this frame, if any */
  }

  /* Feed the per-frame input snapshot before drawing. `wheel` is
   * {delta} (positive = away from the user / scroll up) or null. */
  frame(clicks, pointer, isDown, wheel = null) {
    this.clicks = clicks.slice();
    this.pointer = pointer;
    this.isDown = isDown;
    this.wheel = wheel;
    if(this.dragKey && !isDown) this.dragKey = null;
  }

  _hit(x, y, w, h) {
    for(let i = 0; i < this.clicks.length; i++) {
      const c = this.clicks[i];
      if(c.x >= x && c.x <= x + w && c.y >= y && c.y <= y + h) {
        this.clicks.splice(i, 1); /* consume so only one widget reacts */
        return true;
      }
    }
    return false;
  }

  hover(x, y, w, h) {
    const p = this.pointer;
    return p.x >= x && p.x <= x + w && p.y >= y && p.y <= y + h;
  }

  button(surface, x, y, w, h, label, opts = {}) {
    const hovered = this.hover(x, y, w, h);
    const base = opts.active ? Palette.accent : hovered ? Palette.panel2 : Palette.panel;
    surface.rect(x, y, w, h, opts.color || base, true);
    surface.rect(x, y, w, h, Palette.line, false, 1);
    const scale = opts.scale || 0.5;
    const ts = surface.textSize(label, scale);
    surface.text(label, x + (w - ts.w) / 2, y + (h + ts.h) / 2, opts.active ? Palette.bg : Palette.text, scale);
    return !opts.disabled && this._hit(x, y, w, h);
  }

  toggle(surface, x, y, w, h, label, on) {
    return this.button(surface, x, y, w, h, (on ? '[x] ' : '[ ] ') + label, { active: !!on, scale: 0.45 });
  }

  /*
   * A compact iOS-style on/off switch (track + sliding knob) for a
   * paramsSpec() entry of type 'bool' - used instead of trackbar() so a
   * boolean param doesn't get drawn as a slider with only two positions.
   * Click anywhere in the row to flip it. Returns the new boolean value if
   * clicked, else null.
   */
  switchControl(surface, x, y, w, h, spec, value) {
    const on = !!value;
    const trackW = 40, trackH = Math.max(14, h * 0.6);
    const trackY = y + (h - trackH) / 2;
    surface.rect(x, trackY, trackW, trackH, on ? Palette.accent : Palette.panel2, true);
    const knobR = trackH / 2 - 2;
    const knobX = on ? x + trackW - knobR - 2 : x + knobR + 2;
    surface.circle(knobX, trackY + trackH / 2, knobR, Palette.text, true);
    surface.text(`${spec.label ?? spec.key}: ${on ? 'on' : 'off'}`, x + trackW + 10, y + h * 0.66, Palette.textDim, 0.4);
    return this._hit(x, y, w, h) ? !on : null;
  }

  /*
   * A row of radio buttons for a paramsSpec() entry of type 'enum'
   * (spec.options: [{value,label}, ...]) - used instead of trackbar() so
   * picking one of a handful of named choices isn't a slider drag. Value
   * is the option's array index, matching alphaToParam()/formatValue()'s
   * existing enum convention. Returns the new index if a different option
   * was clicked, else null.
   */
  radioGroup(surface, x, y, w, h, spec, value) {
    const label = `${spec.label ?? spec.key}:`;
    surface.text(label, x, y + h * 0.66, Palette.textDim, 0.4);
    const labelW = surface.textSize(label, 0.4).w + 14;
    const options = spec.options || [];
    const cellW = Math.min(140, (w - labelW) / Math.max(1, options.length));
    let picked = null;
    options.forEach((opt, i) => {
      const optLabel = typeof opt === 'string' ? opt : (opt.label ?? String(opt.value ?? opt));
      const cx = x + labelW + i * cellW + 8,
        cy = y + h / 2;
      const selected = (value | 0) === i;
      surface.circle(cx, cy, 6, selected ? Palette.accent : Palette.panel2, true);
      surface.circle(cx, cy, 6, Palette.line, false, 1);
      surface.text(optLabel, cx + 12, cy + 4, selected ? Palette.text : Palette.textDim, 0.38);
      if(this._hit(cx - 8, cy - 8, cellW - 4, 16) && !selected) picked = i;
    });
    return picked;
  }

  /* A selectable row (used in lists). Returns true on click. */
  row(surface, x, y, w, h, label, selected) {
    const hovered = this.hover(x, y, w, h);
    surface.rect(x, y, w, h, selected ? Palette.accent2 : hovered ? Palette.panel2 : Palette.panel, true);
    surface.text(label, x + 8, y + h * 0.66, selected ? Palette.bg : Palette.text, 0.45);
    return this._hit(x, y, w, h);
  }

  /* A thumbnail tile with caption + selection ring. The image itself is
   * pasted by the caller; this draws the frame + caption + handles the click. */
  tile(surface, x, y, w, h, caption, selected) {
    surface.rect(x - 2, y - 2, w + 4, h + 18, selected ? Palette.accent : Palette.panel2, false, 2);
    surface.rect(x, y + h, w, 16, Palette.panel, true);
    surface.text(caption, x + 4, y + h + 12, Palette.textDim, 0.38);
    return this._hit(x - 2, y - 2, w + 4, h + 18);
  }

  /*
   * In-Mat/in-nvg replacement for cv.createTrackbar(): a horizontal slider
   * driven by a method's declarative paramsSpec() entry (see
   * vector/base.js: {key,label,type:'int'|'float'|'bool'|'enum',min,max,
   * step,default,options?}). Drawn fresh every frame from `value`, so
   * switching which params are shown (e.g. a different frame's method) needs
   * no window rebuild - just draw a different set next frame.
   *
   * Returns the new value if the drag changed it this frame, else null.
   */
  trackbar(surface, x, y, w, h, spec, value, id) {
    id = id ?? spec.key;
    const alpha = clamp01(paramToAlpha(spec, value));
    const trackY = y + h - 6;
    const handleX = x + alpha * w;
    const owner = this.dragKey === id;

    /* start a drag: a fresh left-button-down landed on the control */
    if(!this.dragKey && this._hit(x, y, w, h)) this.dragKey = id;

    surface.text(`${spec.label ?? spec.key}: ${formatValue(spec, value)}`, x, y + 12, Palette.textDim, 0.4);
    surface.rect(x, trackY, w, 4, Palette.panel2, true);
    surface.circle(handleX, trackY + 2, 7, owner || this.dragKey === id ? Palette.accent : Palette.accent2, true);

    if(this.dragKey === id) {
      const newAlpha = clamp01((this.pointer.x - x) / w);
      const newValue = alphaToParam(spec, newAlpha);
      if(newValue !== value) return newValue;
    } else if(this.wheel && this.hover(x, y, w, h)) {
      const newValue = stepValue(spec, value, Math.sign(this.wheel.delta));
      if(newValue !== value) return newValue;
    }
    return null;
  }
}

function clamp01(v) {
  return Math.max(0, Math.min(1, v));
}

function paramToAlpha(spec, v) {
  if(spec.type === 'bool') return v ? 1 : 0;
  if(spec.type === 'enum') return (v | 0) / Math.max(1, spec.options.length - 1);
  return ((v ?? spec.default) - spec.min) / (spec.max - spec.min || 1);
}

function alphaToParam(spec, a) {
  if(spec.type === 'bool') return a >= 0.5;
  if(spec.type === 'enum') return Math.round(a * (spec.options.length - 1));
  const raw = spec.min + a * (spec.max - spec.min);
  const stepped = spec.step ? Math.round(raw / spec.step) * spec.step : raw;
  return spec.type === 'int' ? Math.round(stepped) : +stepped.toFixed(4);
}

function stepValue(spec, value, dir) {
  if(spec.type === 'bool') return dir > 0;
  if(spec.type === 'enum') return Math.max(0, Math.min(spec.options.length - 1, (value | 0) + dir));
  const step = spec.step || 1;
  const nv = Math.max(spec.min, Math.min(spec.max, value + dir * step));
  return spec.type === 'int' ? Math.round(nv) : +nv.toFixed(4);
}

function formatValue(spec, v) {
  if(spec.type === 'bool') return v ? 'on' : 'off';
  if(spec.type === 'enum') return spec.options?.[v]?.label ?? String(v);
  if(spec.type === 'int') return String(Math.round(v));
  return Number(v).toFixed(4).replace(/0+$/, '').replace(/\.$/, '');
}

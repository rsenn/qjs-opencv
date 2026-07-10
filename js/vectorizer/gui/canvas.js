// gui/canvas.js
//
// A thin immediate-mode drawing surface over a cv.Mat. The whole GUI is drawn
// into one BGR Mat and shown with cv.imshow — this avoids depending on the
// Qt-only createButton/displayOverlay, so the app runs on any HighGUI backend.
// Hit-testing for "buttons" is done in widgets.js against the rects we draw here.

import { Mat, Size, Point, Rect, Scalar, resize, cvtColor, drawRect, drawLine, putText, getTextSize, COLOR_GRAY2BGR, FONT_HERSHEY_SIMPLEX, FILLED } from 'opencv.so';

const FILL = typeof FILLED === 'number' ? FILLED : -1;

export const Palette = {
  bg: [28, 26, 24],
  panel: [44, 41, 38],
  panel2: [58, 54, 50],
  accent: [96, 168, 96], // BGR -> a muted green
  accent2: [70, 130, 200], // BGR -> warm blue
  text: [232, 230, 228],
  textDim: [150, 148, 146],
  line: [80, 76, 72],
  danger: [60, 60, 200],
};

function sc(c) {
  return new Scalar(c[0], c[1], c[2]);
}

export class Canvas {
  constructor(width, height) {
    this.width = width;
    this.height = height;
    this.mat = new Mat(new Size(width, height), 16 /* CV_8UC3 */);
    this.clear(Palette.bg);
  }

  clear(color = Palette.bg) {
    drawRect(this.mat, new Rect(0, 0, this.width, this.height), sc(color), FILL);
  }

  rect(x, y, w, h, color, fill = true, thickness = 1) {
    drawRect(this.mat, new Rect(x | 0, y | 0, w | 0, h | 0), sc(color), fill ? FILL : thickness);
  }

  // Rounded-ish panel (plain rect + lighter top edge for a bit of depth).
  panel(x, y, w, h, color = Palette.panel) {
    this.rect(x, y, w, h, color, true);
    drawLine(this.mat, new Point(x, y), new Point(x + w, y), sc(Palette.line), 1);
  }

  line(x1, y1, x2, y2, color = Palette.line, thickness = 1) {
    drawLine(this.mat, new Point(x1 | 0, y1 | 0), new Point(x2 | 0, y2 | 0), sc(color), thickness);
  }

  text(str, x, y, color = Palette.text, scale = 0.5, thickness = 1) {
    putText(this.mat, String(str), new Point(x | 0, y | 0), FONT_HERSHEY_SIMPLEX, scale, sc(color), thickness);
  }

  textSize(str, scale = 0.5, thickness = 1) {
    try {
      const r = getTextSize(String(str), FONT_HERSHEY_SIMPLEX, scale, thickness);
      // returns { width, height } or [size, baseline]
      if(r && typeof r.width == 'number') return { w: r.width, h: r.height };
      //console.log('r', r);
      if(Array.isArray(r)) {
        const [w, h] = r;
        return { w, h };
      }
      //if (Array.isArray(r)) return { w: r[0].width ?? r[0][0], h: r[0].height ?? r[0][1] };
    } catch(_) {}
    return { w: String(str).length * 8 * scale * 2, h: 12 };
  }

  // Paste an image Mat scaled to fit (w,h), letterboxed, into the canvas.
  paste(src, x, y, w, h) {
    if(!src || src.empty) return;
    let img = src;
    const ch = src.channels() ?? 3;
    if(ch === 1) {
      img = new Mat();
      cvtColor(src, img, COLOR_GRAY2BGR);
    }
    const sw = src.cols ?? src.width,
      sh = src.rows ?? src.height;
    const s = Math.min(w / sw, h / sh);
    const dw = Math.max(1, Math.round(sw * s)),
      dh = Math.max(1, Math.round(sh * s));
    const ox = x + ((w - dw) >> 1),
      oy = y + ((h - dh) >> 1);
    const small = new Mat();
    resize(img, small, new Size(dw, dh));
    try {
      const roi = this.mat(new Rect(ox, oy, dw, dh));
      small.copyTo(roi);
    } catch(_) {}
    return { x: ox, y: oy, w: dw, h: dh };
  }
}

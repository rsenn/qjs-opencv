/*
 * gui/canvas.js
 *
 * A thin immediate-mode drawing surface over a cv.Mat, extending cvWidgets'
 * CvSurface with the one vectorizer-specific primitive (paste - blitting a
 * source frame/preview Mat into the GUI). The whole GUI is drawn into one
 * BGR Mat and shown with cv.imshow - this avoids depending on the Qt-only
 * createButton/displayOverlay, so the app runs on any HighGUI backend.
 * Hit-testing for "buttons" is done in cvWidgets.js's Hud against the rects
 * drawn here.
 */

import { Mat, Size, Rect, CV_8UC3, resize, cvtColor, COLOR_GRAY2BGR } from 'opencv';
import { CvSurface, Palette } from '../../cvWidgets.js';

export { Palette };

export class Canvas extends CvSurface {
  constructor(width, height) {
    super(new Mat(new Size(width, height), CV_8UC3));
    /* width/height are already computed getters on CvSurface (from this.mat) */
    this.clear(Palette.bg);
  }

  clear(color = Palette.bg) {
    this.rect(0, 0, this.width, this.height, color, true);
  }

  /* Paste an image Mat scaled to fit (w,h), letterboxed, into the canvas. */
  paste(src, x, y, w, h) {
    if(!src || src.empty()) return;
    let img = src;
    const ch = src.channels ? src.channels() : 3;
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

  /*
   * Paste a pannable/zoomable crop of src into (x,y,w,h): unlike paste(),
   * this does not scale-to-fit - view.zoom = 1 shows the image at its native
   * resolution, view.panX/panY (image-space pixels) pick which portion is
   * visible. When the visible region (w/zoom x h/zoom) is bigger than the
   * image in some axis, the image doesn't fill (x,y,w,h) in that axis and is
   * centered there instead, same letterboxing look as paste().
   */
  pasteViewport(src, x, y, w, h, view) {
    if(!src || src.empty()) return;
    let img = src;
    const ch = src.channels ? src.channels() : 3;
    if(ch === 1) {
      img = new Mat();
      cvtColor(src, img, COLOR_GRAY2BGR);
    }
    const sw = src.cols ?? src.width,
      sh = src.rows ?? src.height;
    const zoom = view.zoom;
    const visW = w / zoom, visH = h / zoom;
    const srcX = Math.max(0, Math.min(view.panX, Math.max(0, sw - 1)));
    const srcY = Math.max(0, Math.min(view.panY, Math.max(0, sh - 1)));
    const cropW = Math.max(1, Math.min(visW, sw - srcX));
    const cropH = Math.max(1, Math.min(visH, sh - srcY));
    const dstW = Math.max(1, Math.min(w, Math.round(cropW * zoom))),
      dstH = Math.max(1, Math.min(h, Math.round(cropH * zoom)));
    const ox = x + Math.max(0, (w - dstW) >> 1),
      oy = y + Math.max(0, (h - dstH) >> 1);
    const scaled = new Mat();
    try {
      const roiSrc = img(new Rect(Math.round(srcX), Math.round(srcY), Math.round(cropW), Math.round(cropH)));
      resize(roiSrc, scaled, new Size(dstW, dstH));
      const roiDst = this.mat(new Rect(ox, oy, dstW, dstH));
      scaled.copyTo(roiDst);
    } catch(_) {}
    return { x: ox, y: oy, w: dstW, h: dstH };
  }
}

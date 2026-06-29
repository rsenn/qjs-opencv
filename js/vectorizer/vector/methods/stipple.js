// vector/methods/stipple.js
//
// Stippling: sample the image on a grid and emit dots whose radius tracks local
// darkness. Cell colour is the cell's mean (via ROI mean, no per-pixel API).
// Gives an engraving / pointillist look that scales cleanly as SVG.

import { Mat, Rect, mean } from 'opencv.so';

import { VectorMethod } from '../base.js';
import { create, point } from '../../core/vectordata.js';
import { toGray, bgrToHex, release } from '../../cv/convert.js';

export class Stipple extends VectorMethod {
  static id = 'stipple';
  static label = 'Stipple Dots';
  static description = 'Grid of dots sized by local darkness (pointillist / engraving).';

  paramsSpec() {
    return [
      { key: 'cell',    label: 'Cell size',  type: 'int',   min: 3, max: 40, step: 1, default: 8 },
      { key: 'maxR',    label: 'Max radius', type: 'float', min: 0.5, max: 10, step: 0.5, default: 3.5 },
      { key: 'gamma',   label: 'Gamma',      type: 'float', min: 0.3, max: 3, step: 0.1, default: 1.2 },
      { key: 'jitter',  label: 'Jitter',     type: 'float', min: 0, max: 1, step: 0.05, default: 0.35 },
      { key: 'colour',  label: 'Use colour', type: 'bool',  default: 0 },
    ];
  }

  apply(mat, p, meta) {
    const W = meta.width, H = meta.height;
    const gray = toGray(mat);
    const shapes = [];
    const cell = Math.max(2, p.cell);
    for (let y = 0; y < H; y += cell) {
      for (let x = 0; x < W; x += cell) {
        const w = Math.min(cell, W - x), h = Math.min(cell, H - y);
        if (w < 1 || h < 1) continue;
        let lum = 128;
        try { lum = mean(gray(new Rect(x, y, w, h)))[0]; } catch (_) {}
        const dark = Math.pow(1 - lum / 255, p.gamma);   // 0 (light) .. 1 (dark)
        if (dark < 0.04) continue;
        const r = dark * p.maxR;
        const jx = (Math.random() - 0.5) * cell * p.jitter;
        const jy = (Math.random() - 0.5) * cell * p.jitter;
        const fill = p.colour ? sampleColour(mat, x, y, w, h) : '#111111';
        shapes.push(point([x + w / 2 + jx, y + h / 2 + jy], r, { fill, stroke: null }));
      }
    }
    release(gray);
    return create(W, H, { shapes });
  }
}

function sampleColour(mat, x, y, w, h) {
  try { return bgrToHex(mean(mat(new Rect(x, y, w, h)))); } catch (_) { return '#111111'; }
}

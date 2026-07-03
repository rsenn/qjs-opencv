// vector/methods/fast-line.js
//
// FastLineDetector — faster, fewer spurious segments than LSD. Same VectorData
// output (line shapes), so it is interchangeable downstream.

import { Mat, FastLineDetector } from 'opencv.so';

import { VectorMethod } from '../base.js';
import { create } from '../../core/vectordata.js';
import { toGray, linesToShapes, release } from '../../cv/convert.js';

export class FastLines extends VectorMethod {
  static id = 'fld';
  static label = 'Fast Line Detector';
  static description = 'Fast line segments (FLD), cleaner than LSD.';

  paramsSpec() {
    return [
      { key: 'lengthThresh', label: 'Length thresh', type: 'int',   min: 5, max: 100, step: 1, default: 20 },
      { key: 'distThresh',   label: 'Dist thresh',   type: 'float', min: 0.5, max: 5, step: 0.1, default: 1.41 },
      { key: 'strokeW',      label: 'Stroke width',  type: 'float', min: 0.2, max: 3, step: 0.1, default: 0.9 },
      { key: 'cannyTh1',      label: 'Canny threshold 1',  type: 'int', min: 0, max: 100, step: 1, default: 50 },
    { key: 'cannyTh2',      label: 'Canny threshold 2',  type: 'int', min: 0, max: 100, step: 1, default: 50 },
    { key: 'cannyAp',      label: 'Canny aperture',  type: 'int', min: 3, max: 7, step: 2, default: 3 },
    ];
  }

  apply(mat, p, meta) {
    const gray = toGray(mat);
    // Constructor args follow OpenCV's FastLineDetector(length_threshold,
    // distance_threshold, canny_th1, canny_th2, canny_aperture, do_merge).
    const fld = new FastLineDetector(p.lengthThresh, p.distThresh, p.cannyTh1, p.cannyTh2, p.cannyAp, false);
    const linesMat = new Mat();
    fld.detect(gray, linesMat);
    const rows = [];
    try { for (const r of linesMat) rows.push(Array.from(r)); } catch (_) {}
    const shapes = linesToShapes(rows, { stroke: '#141414', strokeWidth: p.strokeW, fill: null });
    release(gray, linesMat);
    return create(meta.width, meta.height, { shapes });
  }
}

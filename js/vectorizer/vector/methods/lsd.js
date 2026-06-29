// vector/methods/lsd.js
//
// LineSegmentDetector (LSD). Detects line segments directly from the grayscale
// image — denser and more organic than Hough. Requires the NONFREE modules in
// the OpenCV build (per the qjs-opencv reference); if unavailable, the method
// throws and the GUI shows the error in the preview pane.

import { Mat, LineSegmentDetector } from 'opencv.so';

import { VectorMethod } from '../base.js';
import { create } from '../../core/vectordata.js';
import { toGray, linesToShapes, release } from '../../cv/convert.js';

export class LSDLines extends VectorMethod {
  static id = 'lsd';
  static label = 'Line Segment Detector';
  static description = 'Dense organic line segments via LSD (needs NONFREE build).';

  paramsSpec() {
    return [
      { key: 'minLen',  label: 'Min length',   type: 'int',   min: 0, max: 200, step: 1, default: 12 },
      { key: 'strokeW', label: 'Stroke width', type: 'float', min: 0.2, max: 3, step: 0.1, default: 0.8 },
    ];
  }

  apply(mat, p, meta) {
    const gray = toGray(mat);
    const lsd = new LineSegmentDetector();
    const linesMat = new Mat();
    lsd.detect(gray, linesMat);
    const rows = [];
    try { for (const r of linesMat) rows.push(Array.from(r)); } catch (_) {}
    const filtered = rows.filter((l) => Math.hypot(l[2] - l[0], l[3] - l[1]) >= p.minLen);
    const shapes = linesToShapes(filtered, { stroke: '#141414', strokeWidth: p.strokeW, fill: null });
    release(gray, linesMat);
    return create(meta.width, meta.height, { shapes });
  }
}

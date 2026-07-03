// vector/methods/canny-contours.js
//
// The required starter: Canny edge detection followed by findContours, emitted
// as stroked polylines. Good for line-art / engraving looks.

import {
  Mat, Size, GaussianBlur, Canny, findContours,
  RETR_LIST, CHAIN_APPROX_SIMPLE,
} from 'opencv.so';

import { VectorMethod } from '../base.js';
import { create } from '../../core/vectordata.js';
import { toGray, contoursToShapes, release } from '../../cv/convert.js';

export class CannyContours extends VectorMethod {
  static id = 'canny';
  static label = 'Canny + Contours';
  static description = 'Canny edges traced into stroked polylines (line-art).';

  paramsSpec() {
    return [
      { key: 'blur',     label: 'Blur (odd px)', type: 'int',   min: 0, max: 15, step: 2, default: 3 },
      { key: 'thresh1',  label: 'Canny low',     type: 'int',   min: 0, max: 255, step: 1, default: 50 },
      { key: 'thresh2',  label: 'Canny high',    type: 'int',   min: 0, max: 255, step: 1, default: 150 },
      { key: 'epsilon',  label: 'Simplify',      type: 'float', min: 0, max: 0.02, step: 0.0005, default: 0.002 },
      { key: 'minLen',   label: 'Min points',    type: 'int',   min: 2, max: 50, step: 1, default: 4 },
      { key: 'strokeW',  label: 'Stroke width',  type: 'float', min: 0.2, max: 4, step: 0.1, default: 1 },
    ];
  }

  apply(mat, p, meta) {
    const tick = meta.onProgress || (() => {});
    const gray = toGray(mat);
    if (p.blur >= 3) {
      const k = p.blur % 2 ? p.blur : p.blur + 1;
      GaussianBlur(gray, gray, new Size(k, k), 0);
    }
    tick(0.25);
    const edges = new Mat();
    Canny(gray, edges, p.thresh1, p.thresh2);
    tick(0.55);
    const contours = findContours(edges, RETR_LIST, CHAIN_APPROX_SIMPLE)[0];
    tick(0.75);
    const shapes = contoursToShapes(contours, {
      mode: 'stroke',
      epsilon: p.epsilon,
      minPoints: p.minLen,
      style: { stroke: '#101010', strokeWidth: p.strokeW, fill: null },
    });
    release(gray, edges);
    return create(meta.width, meta.height, { shapes });
  }
}

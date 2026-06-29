// vector/methods/hough-lines.js
//
// Probabilistic Hough transform on Canny edges -> straight line segments.
// Good for architectural / geometric subjects.

import { Mat, Size, GaussianBlur, Canny, HoughLinesP } from 'opencv.so';

import { VectorMethod } from '../base.js';
import { create } from '../../core/vectordata.js';
import { toGray, linesToShapes, release } from '../../cv/convert.js';

export class HoughLines extends VectorMethod {
  static id = 'hough';
  static label = 'Hough Lines';
  static description = 'Straight segments via probabilistic Hough on Canny edges.';

  paramsSpec() {
    return [
      { key: 'thresh1', label: 'Canny low',   type: 'int',   min: 0, max: 255, step: 1, default: 50 },
      { key: 'thresh2', label: 'Canny high',  type: 'int',   min: 0, max: 255, step: 1, default: 150 },
      { key: 'votes',   label: 'Votes',       type: 'int',   min: 10, max: 300, step: 1, default: 60 },
      { key: 'minLen',  label: 'Min length',  type: 'int',   min: 5, max: 300, step: 1, default: 30 },
      { key: 'maxGap',  label: 'Max gap',     type: 'int',   min: 0, max: 100, step: 1, default: 10 },
      { key: 'strokeW', label: 'Stroke width', type: 'float', min: 0.2, max: 4, step: 0.1, default: 1.2 },
    ];
  }

  apply(mat, p, meta) {
    const gray = toGray(mat);
    GaussianBlur(gray, gray, new Size(3, 3), 0);
    const edges = new Mat();
    Canny(gray, edges, p.thresh1, p.thresh2);
    const linesMat = new Mat();
    HoughLinesP(edges, linesMat, 1, Math.PI / 180, p.votes, p.minLen, p.maxGap);
    const shapes = linesToShapes(iterRows(linesMat), {
      stroke: '#101010', strokeWidth: p.strokeW, fill: null,
    });
    release(gray, edges, linesMat);
    return create(meta.width, meta.height, { shapes });
  }
}

// HoughLinesP returns an N x 1 x 4 Mat; normalize to [[x1,y1,x2,y2]...].
function iterRows(m) {
  const out = [];
  try { for (const row of m) out.push(Array.from(row)); } catch (_) {}
  return out;
}

// vector/methods/threshold-contours.js
//
// (Adaptive) thresholding into filled region polygons, preserving holes via the
// contour hierarchy and even-odd fill. Good for silhouettes / stencils.

import {
  Mat, Size, GaussianBlur, threshold, adaptiveThreshold, findContours,
  THRESH_BINARY, THRESH_BINARY_INV, THRESH_OTSU,
  ADAPTIVE_THRESH_GAUSSIAN_C, RETR_CCOMP, CHAIN_APPROX_SIMPLE,
} from 'opencv.so';

import { VectorMethod } from '../base.js';
import { create } from '../../core/vectordata.js';
import { toGray, hierarchyToPaths, release } from '../../cv/convert.js';

export class ThresholdContours extends VectorMethod {
  static id = 'threshold';
  static label = 'Threshold Regions';
  static description = 'Filled silhouettes from global/Otsu/adaptive threshold (keeps holes).';

  paramsSpec() {
    return [
      { key: 'method', label: 'Method', type: 'enum', default: 0,
        options: [{ value: 0, label: 'Global' }, { value: 1, label: 'Otsu' }, { value: 2, label: 'Adaptive' }] },
      { key: 'thresh',    label: 'Threshold',  type: 'int',   min: 0, max: 255, step: 1, default: 128 },
      { key: 'block',     label: 'Adapt block', type: 'int',  min: 3, max: 51, step: 2, default: 15 },
      { key: 'C',         label: 'Adapt C',    type: 'int',   min: -20, max: 20, step: 1, default: 4 },
      { key: 'invert',    label: 'Invert',     type: 'bool',  default: 1 },
      { key: 'epsilon',   label: 'Simplify',   type: 'float', min: 0, max: 0.02, step: 0.0005, default: 0.0015 },
      { key: 'minArea',   label: 'Min area',   type: 'int',   min: 0, max: 5000, step: 25, default: 60 },
    ];
  }

  apply(mat, p, meta) {
    const gray = toGray(mat);
    GaussianBlur(gray, gray, new Size(3, 3), 0);
    const bin = new Mat();
    const type = p.invert ? THRESH_BINARY_INV : THRESH_BINARY;
    if (p.method === 2) {
      const block = p.block % 2 ? p.block : p.block + 1;
      adaptiveThreshold(gray, bin, 255, ADAPTIVE_THRESH_GAUSSIAN_C, type, block, p.C);
    } else if (p.method === 1) {
      threshold(gray, bin, 0, 255, type | THRESH_OTSU);
    } else {
      threshold(gray, bin, p.thresh, 255, type);
    }
    const [contours, hierarchy] = findContours(bin, RETR_CCOMP, CHAIN_APPROX_SIMPLE);
    let shapes = [...hierarchyToPaths(contours, hierarchy, {
      epsilon: p.epsilon,
      style: { stroke: null, fill: '#1a1a1a' },
    })];
    if (p.minArea) shapes = shapes.filter((s) => approxPathArea(s) >= p.minArea);
    release(gray, bin);
    return create(meta.width, meta.height, { shapes });
  }
}

function approxPathArea(shape) {
  const ring = shape.subpaths && shape.subpaths[0];
  if (!ring || ring.length < 3) return 0;
  let s = 0;
  for (let i = 0, n = ring.length; i < n; i++) {
    const [x1, y1] = ring[i], [x2, y2] = ring[(i + 1) % n];
    s += x1 * y2 - x2 * y1;
  }
  return Math.abs(s) / 2;
}

// vector/methods/skeleton.js
//
// Morphological skeletonization -> centerline polylines. Turns thick strokes
// (handwriting, ink, logos) into single-stroke vector paths. Uses the binding's
// `skeletonization` extra; if absent, swap in an erode/dilate thinning loop.

import {
  Mat, MatVector, Size, GaussianBlur, threshold, skeletonization, findContours,
  THRESH_BINARY, THRESH_BINARY_INV, THRESH_OTSU,
  RETR_LIST, CHAIN_APPROX_NONE,
} from 'opencv';

import { VectorMethod } from '../base.js';
import { create } from '../../core/vectordata.js';
import { toGray, contoursToShapes, release } from '../../cv/convert.js';

export class Skeleton extends VectorMethod {
  static id = 'skeleton';
  static label = 'Skeleton Centerlines';
  static description = 'Thin shapes to 1px skeleton, trace as single-stroke paths.';

  paramsSpec() {
    return [
      { key: 'thresh',  label: 'Threshold',   type: 'int',   min: 0, max: 255, step: 1, default: 128 },
      { key: 'otsu',    label: 'Otsu auto',   type: 'bool',  default: 1 },
      { key: 'invert',  label: 'Invert',      type: 'bool',  default: 1 },
      { key: 'epsilon', label: 'Simplify',    type: 'float', min: 0, max: 0.01, step: 0.0002, default: 0.001 },
      { key: 'strokeW', label: 'Stroke width', type: 'float', min: 0.3, max: 3, step: 0.1, default: 1 },
    ];
  }

  apply(mat, p, meta) {
    const tick = meta.tick || (() => {});
    const gray = toGray(mat);
    GaussianBlur(gray, gray, new Size(3, 3), 0);
    const bin = new Mat();
    const type = p.invert ? THRESH_BINARY_INV : THRESH_BINARY;
    if (p.otsu) threshold(gray, bin, 0, 255, type | THRESH_OTSU);
    else threshold(gray, bin, p.thresh, 255, type);
    tick(0.3);

    const skel = new Mat();
    skeletonization(bin, skel);
    tick(0.6);
    const contours = new MatVector(), hierarchy = [];
    findContours(skel, contours, hierarchy, RETR_LIST, CHAIN_APPROX_NONE);
    const shapes = contoursToShapes(contours, {
      mode: 'stroke',
      epsilon: p.epsilon,
      minPoints: 3,
      style: { stroke: '#0c0c0c', strokeWidth: p.strokeW, fill: null },
    });
    release(gray, bin, skel);
    return create(meta.width, meta.height, { shapes });
  }
}

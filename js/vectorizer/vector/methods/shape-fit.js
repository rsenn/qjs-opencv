// vector/methods/shape-fit.js
//
// Fit a geometric primitive to each significant contour: ellipse (fitEllipse),
// rotated rectangle (minAreaRect -> boxPoints) or circle (minEnclosingCircle).
// Produces a clean, abstracted "Bauhaus" reduction of the image.

import {
  Mat, Size, GaussianBlur, threshold, findContours, contourArea,
  fitEllipse, minAreaRect, boxPoints, minEnclosingCircle,
  THRESH_BINARY_INV, THRESH_OTSU, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE,
} from 'opencv.so';

import { VectorMethod } from '../base.js';
import { create, ellipse, polygon, circle } from '../../core/vectordata.js';
import { toGray, pointsOf, colorAt, release } from '../../cv/convert.js';

export class ShapeFit extends VectorMethod {
  static id = 'shapefit';
  static label = 'Shape Fitting';
  static description = 'Abstract each region to a fitted ellipse, rectangle or circle.';

  paramsSpec() {
    return [
      { key: 'shape',  label: 'Primitive', type: 'enum', default: 0,
        options: [{ value: 0, label: 'Ellipse' }, { value: 1, label: 'Rect' }, { value: 2, label: 'Circle' }] },
      { key: 'minArea', label: 'Min area', type: 'int',  min: 20, max: 8000, step: 20, default: 200 },
      { key: 'fill',    label: 'Filled',   type: 'bool', default: 1 },
      { key: 'sample',  label: 'Colour from image', type: 'bool', default: 1 },
    ];
  }

  apply(mat, p, meta) {
    const gray = toGray(mat);
    GaussianBlur(gray, gray, new Size(5, 5), 0);
    const bin = new Mat();
    threshold(gray, bin, 0, 255, THRESH_BINARY_INV | THRESH_OTSU);
    const contours = findContours(bin, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE)[0];

    const shapes = [];
    for (const c of contours) {
      let area = 0;
      try { area = contourArea(c); } catch (_) {}
      if (area < p.minArea) continue;
      const pts = pointsOf(c);
      const cx = avg(pts, 0), cy = avg(pts, 1);
      const fill = p.sample ? colorAt(mat, cx, cy, 2) : '#2a2a2a';
      const style = { fill: p.fill ? fill : null, stroke: p.fill ? null : '#111', strokeWidth: 1 };
      try {
        if (p.shape === 0 && pts.length >= 5) {
          const rr = fitEllipse(c);            // RotatedRect: center,size,angle
          shapes.push(ellipse([rr.center.x, rr.center.y], rr.size.width / 2, rr.size.height / 2, rr.angle, style));
        } else if (p.shape === 1) {
          const rr = minAreaRect(c);
          const box = boxPoints(rr);
          shapes.push(polygon(pointsOf(box), style));
        } else {
          const { center, radius } = circleOf(c);
          shapes.push(circle([center.x, center.y], radius, style));
        }
      } catch (_) { /* skip degenerate contour */ }
    }
    release(gray, bin);
    return create(meta.width, meta.height, { shapes });
  }
}

function avg(pts, i) { return pts.reduce((s, p) => s + p[i], 0) / (pts.length || 1); }

function circleOf(c) {
  // minEnclosingCircle may return {center,radius} or [center,radius].
  const r = minEnclosingCircle(c);
  if (r && r.center) return r;
  return { center: { x: r[0].x ?? r[0][0], y: r[0].y ?? r[0][1] }, radius: r[1] };
}

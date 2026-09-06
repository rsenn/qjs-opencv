// vector/methods/lowpoly-delaunay.js
//
// Low-poly art: scatter feature points (corners + edge samples + jittered grid),
// Delaunay-triangulate them with Subdiv2D, and fill each triangle with the
// colour sampled at its centroid. Produces the classic faceted "low-poly" look.

import {
  Mat, Rect, Point, Subdiv2D, goodFeaturesToTrack, Canny,
} from 'opencv';

import { VectorMethod } from '../base.js';
import { create, polygon } from '../../core/vectordata.js';
import { toGray, pointsOf, colorAt, release } from '../../cv/convert.js';

export class LowPolyDelaunay extends VectorMethod {
  static id = 'lowpoly';
  static label = 'Low-Poly (Delaunay)';
  static description = 'Feature points triangulated into colour-filled facets.';

  paramsSpec() {
    return [
      { key: 'maxPts',  label: 'Max points',  type: 'int',   min: 50, max: 3000, step: 50, default: 600 },
      { key: 'quality', label: 'Quality',     type: 'float', min: 0.001, max: 0.1, step: 0.001, default: 0.01 },
      { key: 'minDist', label: 'Min spacing', type: 'int',   min: 2, max: 40, step: 1, default: 8 },
      { key: 'grid',    label: 'Grid points', type: 'int',   min: 0, max: 40, step: 1, default: 12 },
      { key: 'stroke',  label: 'Edge stroke', type: 'bool',  default: 0 },
    ];
  }

  async apply(mat, p, meta) {
    const tick = meta.tick || (async () => {});
    const W = meta.width, H = meta.height;
    const gray = toGray(mat);
    const corners = new Mat();
    goodFeaturesToTrack(gray, corners, p.maxPts, p.quality, p.minDist);
    const pts = pointsOf(corners);
    await tick(0.2);

    // Always include the four canvas corners + a jittered grid so the whole
    // frame is tessellated, not just the high-detail areas.
    pts.push([0, 0], [W - 1, 0], [W - 1, H - 1], [0, H - 1]);
    if (p.grid > 0) {
      for (let gy = 0; gy <= p.grid; gy++)
        for (let gx = 0; gx <= p.grid; gx++) {
          const jx = (Math.random() - 0.5) * (W / p.grid) * 0.6;
          const jy = (Math.random() - 0.5) * (H / p.grid) * 0.6;
          pts.push([clamp(gx * W / p.grid + jx, 0, W - 1), clamp(gy * H / p.grid + jy, 0, H - 1)]);
        }
    }

    const subdiv = new Subdiv2D(new Rect(0, 0, W, H));
    for (let i = 0; i < pts.length; i++) {
      const [x, y] = pts[i];
      try { subdiv.insert(new Point(x, y)); } catch (_) {}
      if (i % 300 === 299) await tick(0.2 + 0.4 * (i / pts.length));
    }
    const tris = [];   // rows of [x1,y1,x2,y2,x3,y3] - out-param, like findContours
    subdiv.getTriangleList(tris);
    await tick(0.65);

    const shapes = [];
    for (let i = 0; i < tris.length; i++) {
      const t = tris[i];
      const a = [t[0], t[1]], b = [t[2], t[3]], c = [t[4], t[5]];
      if (![a, b, c].every((q) => q[0] >= 0 && q[1] >= 0 && q[0] <= W && q[1] <= H)) continue;
      const cx = (a[0] + b[0] + c[0]) / 3, cy = (a[1] + b[1] + c[1]) / 3;
      const fill = colorAt(mat, cx, cy, 1);
      shapes.push(polygon([a, b, c], {
        fill, stroke: p.stroke ? fill : null, strokeWidth: 0.5,
      }));
      if (i % 300 === 299) await tick(0.65 + 0.3 * (i / tris.length));
    }
    release(gray, corners);
    return create(W, H, { shapes });
  }
}

function clamp(v, lo, hi) { return Math.max(lo, Math.min(hi, v)); }

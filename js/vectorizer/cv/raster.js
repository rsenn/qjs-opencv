// cv/raster.js
//
// GUI-only: rasterize VectorData onto a cv.Mat so it can be shown in the
// HighGUI preview/composite panes. It reuses core flatten()/apply(), so the
// raster preview matches the exported SVG exactly (including baked perspective).

import { Point, Scalar, drawLine, drawCircle, drawPolygon } from 'opencv.so';
import { flatten } from '../core/vectordata.js';
import { apply } from '../core/geometry.js';

export function hexToBGR(hex, fallback = [40, 40, 40]) {
  if(!hex || hex[0] !== '#' || hex.length < 7) return fallback;
  const r = parseInt(hex.slice(1, 3), 16);
  const g = parseInt(hex.slice(3, 5), 16);
  const b = parseInt(hex.slice(5, 7), 16);
  return [b, g, r];
}

// Draw a whole VectorData. If H is provided, every point is transformed (used
// for the stage-4 composite preview, mirroring the baked SVG path).
export function drawVectorData(mat, vd, H = null, opts = {}) {
  const xf = H ? p => apply(H, p) : p => p;
  const scaleStroke = opts.strokeScale || 1;

  for(const shape of vd.shapes) {
    const f = flatten(shape);
    const st = f.style;
    const strokeC = st.stroke ? new Scalar(...hexToBGR(st.stroke)) : null;
    const fillC = st.fill ? new Scalar(...hexToBGR(st.fill)) : null;
    const thick = Math.max(1, Math.round((st.strokeWidth || 1) * scaleStroke));

    if(f.isPoint) {
      const [x, y] = xf(f.rings[0][0]);
      drawCircle(mat, new Point(x | 0, y | 0), Math.max(1, f.r | 0), fillC || strokeC || new Scalar(20, 20, 20), -1);
      continue;
    }

    for(const ring of f.rings) {
      const pts = ring.map(p => {
        const q = xf(p);
        return new Point(q[0] | 0, q[1] | 0);
      });
      if(pts.length < 2) continue;

      if(f.closed && fillC) {
        let filled = false;
        try {
          drawPolygon(mat, pts, fillC, -1);
          filled = true;
        } catch(_) {}
        if(!filled) strokeRing(mat, pts, fillC, 1, true);
        if(strokeC) strokeRing(mat, pts, strokeC, thick, true);
      } else {
        strokeRing(mat, pts, strokeC || new Scalar(20, 20, 20), thick, f.closed);
      }
    }
  }
}

function strokeRing(mat, pts, color, thick, closed) {
  for(let i = 0; i < pts.length - 1; i++) drawLine(mat, pts[i], pts[i + 1], color, thick);
  if(closed && pts.length > 2) drawLine(mat, pts[pts.length - 1], pts[0], color, thick);
}

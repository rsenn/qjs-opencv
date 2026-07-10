// cv/convert.js
//
// The bridge between OpenCV outputs and our generic VectorData. Every binding
// quirk about *how* a cv.Contour or cv.Line iterates is contained here, so the
// vector methods stay readable and the rest of the app never sees a cv.* type.
//
// Named imports only, no `cv.` prefix (house style).

import { Mat, Rect, cvtColor, COLOR_BGR2GRAY, approxPolyDP, arcLength, mean } from 'opencv.so';

import { polyline, polygon, path, line } from '../core/vectordata.js';

// Robustly pull [[x,y]...] out of a cv.Contour / point container. The binding
// may yield cv.Point objects, [x,y] arrays, or offset TypedArrays — handle all.
export function pointsOf(contour) {
  const out = [];
  for(const p of contour) {
    if(p == null) continue;
    if(typeof p.x === 'number') out.push([p.x, p.y]);
    else if(typeof p[0] === 'number') out.push([p[0], p[1]]);
    else if(p.length >= 2) out.push([Number(p[0]), Number(p[1])]);
  }
  return out;
}

// Ensure a single-channel grayscale Mat (clone so the source is never touched).
export function toGray(src) {
  const ch = src.channels ? src.channels() : 3;
  if(ch === 1) return src.clone ? src.clone() : src;
  const gray = new Mat();
  cvtColor(src, gray, COLOR_BGR2GRAY);
  return gray;
}

// Douglas–Peucker simplify a contour, returning a fresh point array.
export function simplify(contour, epsilonFrac) {
  if(!epsilonFrac) return pointsOf(contour);
  const peri = arcLength(contour, true);
  const approx = new Mat();
  approxPolyDP(contour, approx, epsilonFrac * peri, true);
  return pointsOf(approx);
}

// Convert an array of contours into VectorData shapes.
//   mode 'stroke'  -> open/closed polylines (edge art)
//   mode 'fill'    -> filled polygons (region art)
export function contoursToShapes(contours, opts = {}) {
  const { mode = 'stroke', style = {}, epsilon = 0, minPoints = 2, minArea = 0 } = opts;
  const shapes = [];
  for(const c of contours) {
    let pts = simplify(c, epsilon);
    if(pts.length < minPoints) continue;
    if(minArea && polyArea(pts) < minArea) continue;
    shapes.push(mode === 'fill' ? polygon(pts, style) : polyline(pts, style));
  }
  return shapes;
}

// Build a single even-odd <path> from a contour hierarchy (outer + holes).
// hierarchy rows are [next, prev, firstChild, parent] as OpenCV returns.
export function hierarchyToPaths(contours, hierarchy, opts = {}) {
  const { style = {}, epsilon = 0 } = opts;
  const rows = hierarchyRows(hierarchy, contours.length);
  const shapes = [];
  const taken = new Set();
  for(let i = 0; i < contours.length; i++) {
    if(taken.has(i)) continue;
    if(rows[i] && rows[i][3] !== -1) continue; // not a top-level outer
    const subpaths = [simplify(contours[i], epsilon)];
    taken.add(i);
    let child = rows[i] ? rows[i][2] : -1; // firstChild
    while(child !== -1 && child < contours.length) {
      subpaths.push(simplify(contours[child], epsilon));
      taken.add(child);
      child = rows[child][0]; // next sibling
    }
    shapes.push(
      path(
        subpaths.filter(s => s.length >= 3),
        true,
        style,
      ),
    );
  }
  return shapes.length ? shapes : contoursToShapes(contours, { mode: 'fill', style, epsilon });
}

function hierarchyRows(h, n) {
  if(!h) return null;
  // hierarchy may be a Mat (1 x n x 4) or nested arrays; normalize to rows.
  try {
    const flat = [];
    for(const r of h) for (const v of r) flat.push(Number(v));
    if(flat.length >= n * 4) {
      const rows = [];
      for(let i = 0; i < n; i++) rows.push(flat.slice(i * 4, i * 4 + 4));
      return rows;
    }
  } catch(_) {
    /* fall through */
  }
  return null;
}

// Convert detected line segments into VectorData line shapes.
// Accepts an array of cv.Line / [x1,y1,x2,y2] / nested.
export function linesToShapes(lines, style = {}) {
  const shapes = [];
  for(const l of lines || []) {
    let a, b;
    if(l && typeof l.x1 === 'number') {
      a = [l.x1, l.y1];
      b = [l.x2, l.y2];
    } else if(l && l.length >= 4) {
      a = [l[0], l[1]];
      b = [l[2], l[3]];
    } else continue;
    shapes.push(line(a, b, style));
  }
  return shapes;
}

export function polyArea(pts) {
  let s = 0;
  for(let i = 0, n = pts.length; i < n; i++) {
    const [x1, y1] = pts[i],
      [x2, y2] = pts[(i + 1) % n];
    s += x1 * y2 - x2 * y1;
  }
  return Math.abs(s) / 2;
}

// BGR triple (or cv.Scalar) -> '#rrggbb'.
export function bgrToHex(s) {
  const b = clamp255(s[0]),
    g = clamp255(s[1]),
    r = clamp255(s[2]);
  return '#' + [r, g, b].map(v => v.toString(16).padStart(2, '0')).join('');
}
function clamp255(v) {
  return Math.max(0, Math.min(255, Math.round(v || 0)));
}

// Average colour over a mask region -> '#rrggbb'. `mean` is core OpenCV.
export function meanColorMasked(src, mask) {
  try {
    return bgrToHex(mean(src, mask));
  } catch(_) {
    return '#888888';
  }
}

// Colour at (x,y) via a tiny ROI mean (avoids per-pixel binding APIs).
export function colorAt(src, x, y, r = 1) {
  const w = src.cols ?? src.width ?? 0;
  const h = src.rows ?? src.height ?? 0;
  const x0 = Math.max(0, Math.min((w || 1) - 1, Math.round(x) - r));
  const y0 = Math.max(0, Math.min((h || 1) - 1, Math.round(y) - r));
  const rw = Math.min((w || 1) - x0, 2 * r + 1);
  const rh = Math.min((h || 1) - y0, 2 * r + 1);
  try {
    const roi = src(new Rect(x0, y0, rw, rh)); // mat(Rect) -> ROI view
    return bgrToHex(mean(roi));
  } catch(_) {
    return '#888888';
  }
}

// Free a Mat if the binding exposes an explicit destructor (finalizers also
// handle this, but freeing eagerly keeps memory flat in long GUI sessions).
export function release(...mats) {
  for(const m of mats) {
    try {
      if(m && typeof m.delete === 'function') m.delete();
    } catch(_) {}
  }
}

// core/vectordata.js
//
// The universal vector representation. This is the *contract* that decouples
// the three worlds: vectorization methods PRODUCE VectorData, the projector
// TRANSFORMS it, and the SVG composer CONSUMES it. Nothing here imports OpenCV.
//
// VectorData = {
//   width, height,                 // intrinsic image-space dimensions
//   background: '#rrggbb' | null,  // optional backdrop swatch
//   shapes: Shape[]
// }
//
// Shape (discriminated by .kind), all coordinates in image space:
//   { kind:'polyline', points:[[x,y]...], style }
//   { kind:'polygon',  points:[[x,y]...], style }                 // closed
//   { kind:'path',     subpaths:[[[x,y]...]...], evenodd:bool, style } // holes
//   { kind:'line',     a:[x,y], b:[x,y], style }
//   { kind:'circle',   c:[x,y], r, style }
//   { kind:'ellipse',  c:[x,y], rx, ry, angle, style }            // angle deg
//   { kind:'point',    p:[x,y], r, style }
//
// style = { stroke:'#rrggbb'|null, strokeWidth, fill:'#rrggbb'|null, opacity }

export function emptyStyle(over = {}) {
  return Object.assign({ stroke: '#000000', strokeWidth: 1, fill: null, opacity: 1 }, over);
}

export function create(width, height, over = {}) {
  return Object.assign({ width, height, background: null, shapes: [] }, over);
}

export function polyline(points, style) {
  return { kind: 'polyline', points, style: emptyStyle(style) };
}
export function polygon(points, style) {
  return { kind: 'polygon', points, style: emptyStyle(style) };
}
export function path(subpaths, evenodd, style) {
  return { kind: 'path', subpaths, evenodd: !!evenodd, style: emptyStyle(style) };
}
export function line(a, b, style) {
  return { kind: 'line', a, b, style: emptyStyle(style) };
}
export function circle(c, r, style) {
  return { kind: 'circle', c, r, style: emptyStyle(style) };
}
export function ellipse(c, rx, ry, angle, style) {
  return { kind: 'ellipse', c, rx, ry, angle, style: emptyStyle(style) };
}
export function point(p, r, style) {
  return { kind: 'point', p, r: r ?? 1, style: emptyStyle(style) };
}

// Flatten any shape into one or more point-rings, so a projector can apply a
// homography uniformly (perspective maps circles/ellipses to non-conics, so we
// sample them). Returns { rings: [[[x,y]...]...], closed: bool, style }.
export function flatten(shape, samples = 48) {
  const s = shape.style;
  switch (shape.kind) {
    case 'polyline':
      return { rings: [shape.points], closed: false, style: s };
    case 'polygon':
      return { rings: [shape.points], closed: true, style: s };
    case 'path':
      return { rings: shape.subpaths, closed: true, evenodd: shape.evenodd, style: s };
    case 'line':
      return { rings: [[shape.a, shape.b]], closed: false, style: s };
    case 'point':
      return { rings: [[shape.p]], closed: false, style: s, isPoint: true, r: shape.r };
    case 'circle':
      return { rings: [sampleEllipse(shape.c, shape.r, shape.r, 0, samples)], closed: true, style: s };
    case 'ellipse':
      return { rings: [sampleEllipse(shape.c, shape.rx, shape.ry, ((shape.angle || 0) * Math.PI) / 180, samples)], closed: true, style: s };
    default:
      return { rings: [], closed: false, style: s };
  }
}

function sampleEllipse(c, rx, ry, rot, n) {
  const pts = [];
  const cs = Math.cos(rot),
    sn = Math.sin(rot);
  for(let i = 0; i < n; i++) {
    const a = (i / n) * 2 * Math.PI;
    const x = rx * Math.cos(a),
      y = ry * Math.sin(a);
    pts.push([c[0] + x * cs - y * sn, c[1] + x * sn + y * cs]);
  }
  return pts;
}

// Total shape count + a cheap bbox, handy for stage thumbnails / overlays.
export function bbox(vd) {
  let minX = Infinity,
    minY = Infinity,
    maxX = -Infinity,
    maxY = -Infinity;
  for(const sh of vd.shapes)
    for(const ring of flatten(sh).rings)
      for(const [x, y] of ring) {
        if(x < minX) minX = x;
        if(y < minY) minY = y;
        if(x > maxX) maxX = x;
        if(y > maxY) maxY = y;
      }
  if(!isFinite(minX)) return { x: 0, y: 0, w: vd.width, h: vd.height };
  return { x: minX, y: minY, w: maxX - minX, h: maxY - minY };
}

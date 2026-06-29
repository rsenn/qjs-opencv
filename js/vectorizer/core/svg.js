// core/svg.js
//
// Composes the final SVG from placed VectorData. Pure JS: depends only on
// geometry + vectordata. Affine placements are emitted as a native <g
// transform="matrix(...)"> (keeps the SVG editable); perspective placements are
// baked point-by-point (SVG transforms can't express a homography).

import { apply, isAffine, toSvgMatrix } from './geometry.js';
import { flatten } from './vectordata.js';

function fmt(n) { return Math.abs(n) < 1e-4 ? 0 : +n.toFixed(3); }
function pt([x, y]) { return `${fmt(x)},${fmt(y)}`; }

function styleAttrs(st) {
  const a = [];
  a.push(`fill="${st.fill || 'none'}"`);
  if (st.stroke) { a.push(`stroke="${st.stroke}"`); a.push(`stroke-width="${fmt(st.strokeWidth)}"`); }
  if (st.opacity != null && st.opacity !== 1) a.push(`opacity="${fmt(st.opacity)}"`);
  a.push('stroke-linejoin="round"', 'stroke-linecap="round"');
  return a.join(' ');
}

// Render one shape to an SVG element string. If `H` is null the shape is
// emitted in image space (used inside an affine <g>); otherwise points are
// transformed by H (used for baked perspective placements).
function shapeToSvg(shape, H) {
  const f = flatten(shape);
  const xf = H ? (p) => apply(H, p) : (p) => p;
  const st = f.style;

  if (f.isPoint) {
    const [x, y] = xf(f.rings[0][0]);
    return `<circle cx="${fmt(x)}" cy="${fmt(y)}" r="${fmt(f.r)}" ${styleAttrs(st)}/>`;
  }
  // Single open ring -> polyline; closed rings / multi-ring -> path.
  if (!f.closed && f.rings.length === 1) {
    const d = f.rings[0].map((p) => pt(xf(p))).join(' ');
    return `<polyline points="${d}" ${styleAttrs(st)}/>`;
  }
  const d = f.rings
    .filter((r) => r.length)
    .map((ring) => 'M ' + ring.map((p) => pt(xf(p))).join(' L ') + (f.closed ? ' Z' : ''))
    .join(' ');
  const fr = f.evenodd ? ' fill-rule="evenodd"' : '';
  return `<path d="${d}"${fr} ${styleAttrs(st)}/>`;
}

// Render a single VectorData (no placement) — used for the stage-3 preview pane.
export function renderVectorData(vd, opts = {}) {
  const w = opts.width || vd.width;
  const h = opts.height || vd.height;
  const body = vd.shapes.map((s) => shapeToSvg(s, null)).join('\n  ');
  const bg = vd.background
    ? `<rect width="${w}" height="${h}" fill="${vd.background}"/>\n  ` : '';
  return `<svg xmlns="http://www.w3.org/2000/svg" width="${w}" height="${h}" viewBox="0 0 ${w} ${h}">\n  ${bg}${body}\n</svg>`;
}

// Compose the final document from placements:
//   placement = { vectorData, H (Float64Array(9)), id, opacity? }
export function compose(canvasW, canvasH, placements, opts = {}) {
  const layers = placements.map((pl) => {
    const wrapOpacity = pl.opacity != null && pl.opacity !== 1 ? ` opacity="${fmt(pl.opacity)}"` : '';
    if (isAffine(pl.H)) {
      // Editable: emit native transform, shapes stay in image space.
      const body = pl.vectorData.shapes.map((s) => '  ' + shapeToSvg(s, null)).join('\n');
      return `<g id="${pl.id || ''}"${wrapOpacity} transform="${toSvgMatrix(pl.H)}">\n${body}\n</g>`;
    }
    // Perspective: bake H into every point.
    const body = pl.vectorData.shapes.map((s) => '  ' + shapeToSvg(s, pl.H)).join('\n');
    return `<g id="${pl.id || ''}"${wrapOpacity}>\n${body}\n</g>`;
  });
  const bg = opts.background
    ? `<rect width="${canvasW}" height="${canvasH}" fill="${opts.background}"/>\n` : '';
  return `<svg xmlns="http://www.w3.org/2000/svg" width="${canvasW}" height="${canvasH}" viewBox="0 0 ${canvasW} ${canvasH}">\n${bg}${layers.join('\n')}\n</svg>`;
}

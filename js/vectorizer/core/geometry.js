// core/geometry.js
//
// Pure-JS projective geometry. NO OpenCV dependency on purpose: this is the
// decoupling boundary. Stage 4 (projecting) and the SVG composer transform
// vector points with these 3x3 homographies, so the result is perspective
// correct without ever touching cv.warpPerspective. The GUI may *additionally*
// use cv.warpPerspective for a fast raster preview, but the SVG truth lives here.

// A matrix is a flat row-major Float64Array(9): [a,b,c, d,e,f, g,h,i]
//   | a b c |   maps (x,y) -> ( (a x + b y + c)/w, (d x + e y + f)/w )
//   | d e f |   with w = g x + h y + i
//   | g h i |

export function identity() {
  return new Float64Array([1, 0, 0, 0, 1, 0, 0, 0, 1]);
}

export function multiply(A, B) {
  const M = new Float64Array(9);
  for (let r = 0; r < 3; r++)
    for (let c = 0; c < 3; c++)
      M[r * 3 + c] =
        A[r * 3 + 0] * B[0 * 3 + c] +
        A[r * 3 + 1] * B[1 * 3 + c] +
        A[r * 3 + 2] * B[2 * 3 + c];
  return M;
}

// Apply homography to a point [x,y] -> [x',y'] (perspective divide).
export function apply(H, p) {
  const x = p[0], y = p[1];
  const w = H[6] * x + H[7] * y + H[8] || 1e-12;
  return [(H[0] * x + H[1] * y + H[2]) / w, (H[3] * x + H[4] * y + H[5]) / w];
}

export function translate(tx, ty) {
  return new Float64Array([1, 0, tx, 0, 1, ty, 0, 0, 1]);
}

export function scale(sx, sy) {
  return new Float64Array([sx, 0, 0, 0, sy, 0, 0, 0, 1]);
}

export function rotate(rad) {
  const c = Math.cos(rad), s = Math.sin(rad);
  return new Float64Array([c, -s, 0, s, c, 0, 0, 0, 1]);
}

// Compose a TRS placement around a pivot (image-space center by default).
export function affineTRS(tx, ty, sx, sy, rad, pivotX = 0, pivotY = 0) {
  let M = translate(tx, ty);
  M = multiply(M, translate(pivotX, pivotY));
  M = multiply(M, rotate(rad));
  M = multiply(M, scale(sx, sy));
  M = multiply(M, translate(-pivotX, -pivotY));
  return M;
}

// Solve A x = b for an n x n system (Gaussian elimination, partial pivot).
function solve(A, b, n) {
  const M = A.map((row, i) => row.concat(b[i]));
  for (let col = 0; col < n; col++) {
    let piv = col;
    for (let r = col + 1; r < n; r++)
      if (Math.abs(M[r][col]) > Math.abs(M[piv][col])) piv = r;
    [M[col], M[piv]] = [M[piv], M[col]];
    const d = M[col][col] || 1e-12;
    for (let c = col; c <= n; c++) M[col][c] /= d;
    for (let r = 0; r < n; r++) {
      if (r === col) continue;
      const f = M[r][col];
      for (let c = col; c <= n; c++) M[r][c] -= f * M[col][c];
    }
  }
  return M.map((row) => row[n]);
}

// Homography mapping 4 source points -> 4 destination points.
// src/dst are [[x,y],[x,y],[x,y],[x,y]] (any convex/concave quad). This is the
// pure-JS equivalent of cv.getPerspectiveTransform, kept here for decoupling.
export function getPerspectiveTransform(src, dst) {
  const A = [], b = [];
  for (let i = 0; i < 4; i++) {
    const [x, y] = src[i];
    const [u, v] = dst[i];
    A.push([x, y, 1, 0, 0, 0, -x * u, -y * u]); b.push(u);
    A.push([0, 0, 0, x, y, 1, -x * v, -y * v]); b.push(v);
  }
  const h = solve(A, b, 8);
  return new Float64Array([h[0], h[1], h[2], h[3], h[4], h[5], h[6], h[7], 1]);
}

// Convenience: map an image rect (0,0)-(w,h) onto an arbitrary dst quad.
export function rectToQuad(w, h, quad) {
  return getPerspectiveTransform(
    [[0, 0], [w, 0], [w, h], [0, h]],
    quad,
  );
}

// True if the homography is a pure affine map (bottom row is [0,0,1]).
export function isAffine(H, eps = 1e-9) {
  return Math.abs(H[6]) < eps && Math.abs(H[7]) < eps && Math.abs(H[8] - 1) < eps;
}

// Emit an SVG matrix(...) string for an affine H. (Perspective is baked into
// points by the composer instead.)
export function toSvgMatrix(H) {
  // SVG matrix(a b c d e f): x' = a x + c y + e ; y' = b x + d y + f
  return `matrix(${H[0]} ${H[3]} ${H[1]} ${H[4]} ${H[2]} ${H[5]})`;
}

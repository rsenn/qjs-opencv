import { Mat, Size, Point, CV_8UC1, CV_8UC3, CV_32FC1, CV_32FC3, CV_32S, CV_32SC2, cvtColor, COLOR_BGR2GRAY, COLOR_BGR2Lab, COLOR_Lab2BGR, pyrMeanShiftFiltering, kmeans, KMEANS_PP_CENTERS, TERM_CRITERIA_COUNT, TERM_CRITERIA_EPS, resize, INTER_NEAREST, morphologyEx, getStructuringElement, MORPH_RECT, MORPH_OPEN, MORPH_CLOSE, connectedComponentsWithStats, findContours, RETR_LIST, RETR_CCOMP, CHAIN_APPROX_NONE, CHAIN_APPROX_SIMPLE, bitwise_and, imread, readNetFromONNX, blobFromImage, GaussianBlur, Canny, createCLAHE, ximgproc, psimpl, approxPolyDP, arcLength, PointVectorVector, traceSkeleton, MatVector, } from 'opencv';
import { Processor } from './cvPipeline.js';
import { NumericParam } from './cvParam.js';
import { create as createVectorData } from './vectorizer/core/vectordata.js';
import { contoursToShapes, linesToShapes } from './vectorizer/cv/convert.js';

/**
 * Full-color image to SVG vectorizer (vectorizer.ai style).
 *
 * Pipeline: [ONNX segmentation] -> pyrMeanShiftFiltering -> kmeans in Lab ->
 * upscaled label map -> per-color masks -> morphology -> connected components ->
 * findContours (RETR_CCOMP) -> psimpl cleanup/simplification -> corner detection ->
 * Schneider least-squares cubic bezier fitting -> SVG.
 */
export const defaultOptions = {
  colors: 8, // kmeans K (palette size)
  tolerance: 1.0, // max bezier fitting error, px (in upscaled space)
  minArea: 16, // minimum region area, px^2 at original resolution
  upscale: 2, // integer label-map upscale factor for pseudo-subpixel tracing
  simplify: 'reumann-witkam:1', // method:tolerance for the gentle denoise pass
  cornerThreshold: 100, // interior angle (deg) below which a vertex is a corner
  radialTolerance: 0.75, // pre-cleanup: collapse points closer than this
  model: null, // path to ONNX segmentation model (optional)
  inferSize: 320, // square input size for ONNX inference
  spatialRadius: 12, // pyrMeanShiftFiltering spatial window
  colorRadius: 24, // pyrMeanShiftFiltering color window
  strokeWidth: 0.5, // SVG stroke width (hides seams between adjacent regions)
  verbose: false,
};

function hexColor([b, g, r]) {
  return '#' + [r, g, b].map(v => v.toString(16).padStart(2, '0')).join('');
}

/* ------------------------------------------------------------------ *
 * ColorQuantizer: mean-shift flattening + kmeans in Lab space
 * ------------------------------------------------------------------ */
export class ColorQuantizer {
  constructor(colors = 8, options = {}) {
    this.colors = colors;
    this.spatialRadius = options.spatialRadius ?? defaultOptions.spatialRadius;
    this.colorRadius = options.colorRadius ?? defaultOptions.colorRadius;
  }

  /* quantize(mat: 8UC3 BGR) -> { labelMap: Mat 8UC1, palette: [[b,g,r]], filtered: Mat } */
  quantize(mat) {
    const filtered = new Mat();
    pyrMeanShiftFiltering(mat, filtered, this.spatialRadius, this.colorRadius);

    const lab = new Mat();
    cvtColor(filtered, lab, COLOR_BGR2Lab);

    const { rows, cols } = lab;
    const n = rows * cols;
    const samples = new Mat(new Size(3, n), CV_32FC1);
    const f = new Float32Array(samples.buffer);
    const u = new Uint8Array(lab.buffer);

    for(let i = 0; i < n * 3; i++) f[i] = u[i];

    const k = Math.min(this.colors, n);
    const bestLabels = new Mat(),
      centers = new Mat();

    kmeans(samples, k, bestLabels, [TERM_CRITERIA_COUNT | TERM_CRITERIA_EPS, 20, 1.0], 1, KMEANS_PP_CENTERS, centers);

    const labelMap = new Mat(new Size(cols, rows), CV_8UC1);
    const lm = new Uint8Array(labelMap.buffer);
    const bl = new Int32Array(bestLabels.buffer);

    for(let i = 0; i < n; i++) lm[i] = bl[i];

    /* centers are Kx3 float Lab; convert through a 1xK Lab image to BGR */
    const labPal = new Mat(new Size(k, 1), CV_8UC3);
    const lp = new Uint8Array(labPal.buffer);
    const cf = new Float32Array(centers.buffer);

    for(let i = 0; i < k * 3; i++) lp[i] = Math.max(0, Math.min(255, Math.round(cf[i])));

    const bgrPal = new Mat();
    cvtColor(labPal, bgrPal, COLOR_Lab2BGR);

    const bp = new Uint8Array(bgrPal.buffer);
    const palette = [];

    for(let i = 0; i < k; i++) palette.push([bp[i * 3], bp[i * 3 + 1], bp[i * 3 + 2]]);

    for(let m of [lab, samples, bestLabels, centers, labPal, bgrPal]) m.release();

    return { labelMap, palette, filtered };
  }
}

/* ------------------------------------------------------------------ *
 * OnnxSegmenter: optional dnn-based segmentation (matting or semantic)
 * ------------------------------------------------------------------ */
export class OnnxSegmenter {
  constructor(modelPath, options = {}) {
    this.inferSize = options.inferSize ?? defaultOptions.inferSize;

    try {
      this.net = readNetFromONNX(modelPath);
    } catch(e) {
      throw new Error(`OnnxSegmenter: cannot load ONNX model '${modelPath}': ${e.message}`);
    }

    if(!this.net || this.net.empty()) throw new Error(`OnnxSegmenter: model '${modelPath}' loaded empty`);
  }

  /* segment(mat: 8UC3 BGR) -> [Mat 8UC1 mask (0/255)] at input resolution.
   * Auto-detects: 1 output channel = matting (fg/bg via 0.5 threshold),
   * C > 1 channels = semantic segmentation (argmax per pixel). */
  segment(mat) {
    const s = this.inferSize;
    const blob = new Mat();

    blobFromImage(mat, blob, 1 / 255.0, new Size(s, s), [0, 0, 0, 0], true, false);
    this.net.setInput(blob);

    const out = this.net.forward();
    const data = new Float32Array(out.buffer);
    const px = s * s;

    if(data.length % px != 0) throw new Error(`OnnxSegmenter: unsupported output shape (${data.length} values for ${s}x${s} input)`);

    const classes = data.length / px;
    const planes = [];

    if(classes == 1) {
      /* matting: threshold to foreground / background masks */
      const fg = new Uint8Array(px),
        bg = new Uint8Array(px);

      for(let i = 0; i < px; i++) {
        if(data[i] > 0.5) fg[i] = 255;
        else bg[i] = 255;
      }

      planes.push(fg, bg);
    } else {
      /* semantic segmentation: per-class argmax masks */
      for(let c = 0; c < classes; c++) planes.push(new Uint8Array(px));

      for(let i = 0; i < px; i++) {
        let best = 0,
          bestV = data[i];

        for(let c = 1; c < classes; c++) {
          const v = data[c * px + i];

          if(v > bestV) {
            bestV = v;
            best = c;
          }
        }

        planes[best][i] = 255;
      }
    }

    const masks = [];

    for(let plane of planes) {
      let nonZero = 0;

      for(let i = 0; i < px; i++) if(plane[i]) nonZero++;

      if(nonZero == 0) continue;

      const small = new Mat(new Size(s, s), CV_8UC1);

      new Uint8Array(small.buffer).set(plane);

      const mask = new Mat();

      resize(small, mask, new Size(mat.cols, mat.rows), 0, 0, INTER_NEAREST);
      small.release();
      masks.push(mask);
    }

    blob.release();
    out.release();

    return masks;
  }
}

/* ------------------------------------------------------------------ *
 * ContourTracer: label map -> per-color regions with holes
 * ------------------------------------------------------------------ */
export class ContourTracer {
  constructor(options = {}) {
    this.minArea = options.minArea ?? defaultOptions.minArea;
    this.upscale = options.upscale ?? defaultOptions.upscale;
  }

  /* trace(labelMap: 8UC1, paletteSize, segMasks?) ->
   *   [{ colorIndex, regions: [{ outer: Contour, holes: [Contour], area }] }]
   * Coordinates are in upscaled space. */
  trace(labelMap, paletteSize, segMasks = null) {
    const u = Math.max(1, Math.round(this.upscale));
    let scaled = labelMap;

    if(u > 1) {
      scaled = new Mat();
      resize(labelMap, scaled, new Size(labelMap.cols * u, labelMap.rows * u), 0, 0, INTER_NEAREST);
    }

    const minArea = this.minArea * u * u;
    const size = scaled.size();
    const total = scaled.rows * scaled.cols;
    const lm = new Uint8Array(scaled.buffer);
    const kernel = getStructuringElement(MORPH_RECT, new Size(3, 3));
    let scaledMasks = null;

    if(segMasks) {
      scaledMasks = segMasks.map(m => {
        const sm = new Mat();
        resize(m, sm, size, 0, 0, INTER_NEAREST);
        return sm;
      });
    }

    const result = [];

    for(let colorIndex = 0; colorIndex < paletteSize; colorIndex++) {
      const mask = new Mat(size, CV_8UC1);
      const mb = new Uint8Array(mask.buffer);

      for(let i = 0; i < total; i++) mb[i] = lm[i] == colorIndex ? 255 : 0;

      const regions = [];

      if(scaledMasks) {
        for(let sm of scaledMasks) {
          const sub = new Mat();

          bitwise_and(mask, sm, sub);
          this.#traceMask(sub, kernel, minArea, regions);
          sub.release();
        }
      } else {
        this.#traceMask(mask, kernel, minArea, regions);
      }

      mask.release();
      result.push({ colorIndex, regions });
    }

    if(scaledMasks) for(let sm of scaledMasks) sm.release();
    if(scaled !== labelMap) scaled.release();
    kernel.release();

    return result;
  }

  #traceMask(mask, kernel, minArea, regions) {
    const opened = new Mat(),
      cleaned = new Mat();

    morphologyEx(mask, opened, MORPH_OPEN, kernel);
    morphologyEx(opened, cleaned, MORPH_CLOSE, kernel);
    opened.release();

    /* drop connected components below the area threshold */
    const labels = new Mat(),
      stats = new Mat(),
      centroids = new Mat();
    const nComp = connectedComponentsWithStats(cleaned, labels, stats, centroids, 8, CV_32S, -1);
    const st = new Int32Array(stats.buffer);
    const lb = new Int32Array(labels.buffer);
    const cb = new Uint8Array(cleaned.buffer);
    let kept = 0;

    for(let c = 1; c < nComp; c++) if(st[c * 5 + 4] >= minArea) kept++;

    if(kept > 0) {
      for(let i = 0; i < lb.length; i++) if(cb[i] && st[lb[i] * 5 + 4] < minArea) cb[i] = 0;

      const contours = new MatVector(),
        hierarchy = [];

      findContours(cleaned, contours, hierarchy, RETR_CCOMP, CHAIN_APPROX_NONE, new Point(0, 0));

      for(let i = 0; i < contours.size(); i++) {
        if(hierarchy[i][3] != -1) continue; /* holes are collected via their parent */
        if(contours.get(i).length < 3 || contours.get(i).area < minArea) continue;

        const holes = [];

        for(let j = hierarchy[i][2]; j != -1; j = hierarchy[j][0]) if(contours.get(j).length >= 3) holes.push(contours.get(j));

        regions.push({ outer: contours.get(i), holes, area: contours.get(i).area });
      }
    }

    for(let m of [labels, stats, centroids, cleaned]) m.release();
  }
}

/* ------------------------------------------------------------------ *
 * PolySimplifier: wraps the psimpl-bound Contour.simplify* methods
 * ------------------------------------------------------------------ */
export class PolySimplifier {
  static METHODS = ['reumann-witkam', 'perpendicular-distance', 'douglas-peucker', 'opheim', 'lang', 'nth-point', 'radial-distance'];

  /* spec: 'method:tolerance', e.g. 'reumann-witkam:1' */
  constructor(spec = defaultOptions.simplify) {
    const [method, tol] = String(spec).split(':');
    this.method = method;
    this.tolerance = tol === undefined || tol === '' ? 1 : parseFloat(tol);

    if(!PolySimplifier.METHODS.includes(method)) throw new Error(`PolySimplifier: unknown method '${method}' (available: ${PolySimplifier.METHODS.join(', ')})`);
    if(!Number.isFinite(this.tolerance) || this.tolerance <= 0) throw new Error(`PolySimplifier: invalid tolerance '${tol}'`);
  }

  /* simplify(contour: Contour) -> new Contour */
  simplify(contour) {
    const t = this.tolerance;

    switch (this.method) {
      case 'reumann-witkam':
        return contour.simplifyReumannWitkam(t);
      case 'perpendicular-distance':
        return contour.simplifyPerpendicularDistance(t, 2);
      case 'douglas-peucker':
        return contour.simplifyDouglasPeucker(t);
      case 'opheim':
        return contour.simplifyOpheim(t, t * 10);
      case 'lang':
        return contour.simplifyLang(t, 8);
      case 'nth-point':
        return contour.simplifyNthPoint(Math.max(2, Math.round(t)));
      case 'radial-distance':
        return contour.simplifyRadialDistance(t);
    }
  }
}

/* ------------------------------------------------------------------ *
 * CurveFitter: Schneider least-squares cubic bezier fitting
 * (Graphics Gems "FitCurves"). Pure JS, no OpenCV.
 * ------------------------------------------------------------------ */
export class CurveFitter {
  constructor(tolerance = 1.0, cornerThreshold = 100) {
    this.tolerance = tolerance;
    this.cornerThreshold = cornerThreshold;
  }

  /* Fit a closed contour. rawXY: flat [x0,y0,...] of the dense (radial-distance
   * cleaned) points used for the least-squares fit; simplifiedXY: flat coords of
   * the simplified polyline used only for corner detection.
   * Returns [[p0,c1,c2,p3], ...] with {x,y} points, covering the closed loop. */
  fitClosed(rawXY, simplifiedXY) {
    const pts = CurveFitter.#toPoints(rawXY);
    const n = pts.length;

    if(n < 3) return [];

    const corners = this.detectCorners(simplifiedXY);
    let idx = CurveFitter.#mapToIndices(pts, corners);
    const smoothClosed = idx.length == 0;

    if(smoothClosed) idx = [0];

    idx.sort((a, b) => a - b);

    const out = [];

    for(let s = 0; s < idx.length; s++) {
      const a = idx[s],
        b = idx[(s + 1) % idx.length];
      const run = CurveFitter.#sliceWrap(pts, a, b);

      if(run.length < 2) continue;

      let tHat1, tHat2;

      if(smoothClosed) {
        /* single artificial split: keep the seam tangent-continuous */
        const t = CurveFitter.#normalize(CurveFitter.#sub(pts[(a + 1) % n], pts[(a - 1 + n) % n]));
        tHat1 = t;
        tHat2 = { x: -t.x, y: -t.y };
      } else {
        tHat1 = CurveFitter.#normalize(CurveFitter.#sub(run[1], run[0]));
        tHat2 = CurveFitter.#normalize(CurveFitter.#sub(run[run.length - 2], run[run.length - 1]));
      }

      this.#fitCubic(run, 0, run.length - 1, tHat1, tHat2, out);
    }

    return out;
  }

  /* Corners: vertices of the simplified polyline whose interior angle
   * (measured over a small sliding window) is below cornerThreshold degrees.
   * Returns [{x,y}]. */
  detectCorners(simplifiedXY) {
    const sp = CurveFitter.#toPoints(simplifiedXY);
    const m = sp.length;

    if(m < 4) return [];

    const minDist = 2,
      maxSteps = 4;
    const candidates = [];

    for(let i = 0; i < m; i++) {
      let prev = sp[i],
        next = sp[i];

      for(let s = 1; s <= maxSteps; s++) {
        prev = sp[(i - s + m * maxSteps) % m];
        if(CurveFitter.#dist(prev, sp[i]) >= minDist) break;
      }

      for(let s = 1; s <= maxSteps; s++) {
        next = sp[(i + s) % m];
        if(CurveFitter.#dist(next, sp[i]) >= minDist) break;
      }

      const u = CurveFitter.#sub(prev, sp[i]),
        v = CurveFitter.#sub(next, sp[i]);
      const lu = Math.hypot(u.x, u.y),
        lv = Math.hypot(v.x, v.y);

      if(lu == 0 || lv == 0) continue;

      const cos = Math.max(-1, Math.min(1, (u.x * v.x + u.y * v.y) / (lu * lv)));
      const angle = (Math.acos(cos) * 180) / Math.PI;

      if(angle < this.cornerThreshold) candidates.push({ i, angle, x: sp[i].x, y: sp[i].y });
    }

    /* non-max suppression: keep the sharpest of adjacent candidates */
    const corners = [];

    for(let c = 0; c < candidates.length; c++) {
      const cur = candidates[c];
      const prev = candidates[(c - 1 + candidates.length) % candidates.length];
      const next = candidates[(c + 1) % candidates.length];
      const nearPrev = prev !== cur && (cur.i - prev.i + m) % m <= 1;
      const nearNext = next !== cur && (next.i - cur.i + m) % m <= 1;

      if((nearPrev && prev.angle < cur.angle) || (nearNext && next.angle <= cur.angle)) continue;

      corners.push({ x: cur.x, y: cur.y });
    }

    return corners;
  }

  /* evaluate a cubic bezier [p0,c1,c2,p3] at t (de Casteljau) */
  static evaluate(bez, t) {
    let tmp = bez.map(p => ({ x: p.x, y: p.y }));

    for(let i = 1; i < 4; i++)
      for(let j = 0; j < 4 - i; j++) {
        tmp[j].x = (1 - t) * tmp[j].x + t * tmp[j + 1].x;
        tmp[j].y = (1 - t) * tmp[j].y + t * tmp[j + 1].y;
      }

    return tmp[0];
  }

  #fitCubic(pts, first, last, tHat1, tHat2, out) {
    const nPts = last - first + 1;

    if(nPts == 2) {
      const dist = CurveFitter.#dist(pts[first], pts[last]) / 3;

      out.push([pts[first], CurveFitter.#addScaled(pts[first], tHat1, dist), CurveFitter.#addScaled(pts[last], tHat2, dist), pts[last]]);
      return;
    }

    let u = CurveFitter.#chordLengthParameterize(pts, first, last);
    let bez = CurveFitter.#generateBezier(pts, first, last, u, tHat1, tHat2);
    let { maxError, splitPoint } = CurveFitter.#computeMaxError(pts, first, last, bez, u);
    const tolSq = this.tolerance * this.tolerance;

    if(maxError < tolSq) {
      out.push(bez);
      return;
    }

    if(maxError < tolSq * 4) {
      /* error is close: try Newton-Raphson reparameterization */
      for(let i = 0; i < 4; i++) {
        u = CurveFitter.#reparameterize(pts, first, last, u, bez);
        bez = CurveFitter.#generateBezier(pts, first, last, u, tHat1, tHat2);
        ({ maxError, splitPoint } = CurveFitter.#computeMaxError(pts, first, last, bez, u));

        if(maxError < tolSq) {
          out.push(bez);
          return;
        }
      }
    }

    /* split at max-error point and recurse */
    if(splitPoint <= first || splitPoint >= last) splitPoint = first + ((last - first) >> 1);

    const tHatCenter = CurveFitter.#normalize(CurveFitter.#sub(pts[splitPoint - 1], pts[splitPoint + 1]));

    this.#fitCubic(pts, first, splitPoint, tHat1, tHatCenter, out);
    this.#fitCubic(pts, splitPoint, last, { x: -tHatCenter.x, y: -tHatCenter.y }, tHat2, out);
  }

  static #generateBezier(pts, first, last, u, tHat1, tHat2) {
    const nPts = last - first + 1;
    const A = [];

    for(let i = 0; i < nPts; i++) {
      const t = u[i],
        b1 = 3 * t * (1 - t) * (1 - t),
        b2 = 3 * t * t * (1 - t);

      A.push([
        { x: tHat1.x * b1, y: tHat1.y * b1 },
        { x: tHat2.x * b2, y: tHat2.y * b2 },
      ]);
    }

    const C = [
        [0, 0],
        [0, 0],
      ],
      X = [0, 0];
    const p0 = pts[first],
      p3 = pts[last];

    for(let i = 0; i < nPts; i++) {
      const t = u[i];
      const b0 = (1 - t) ** 3,
        b1 = 3 * t * (1 - t) * (1 - t),
        b2 = 3 * t * t * (1 - t),
        b3 = t ** 3;
      const p = pts[first + i];
      const tmp = {
        x: p.x - (b0 * p0.x + b1 * p0.x + b2 * p3.x + b3 * p3.x),
        y: p.y - (b0 * p0.y + b1 * p0.y + b2 * p3.y + b3 * p3.y),
      };

      C[0][0] += A[i][0].x * A[i][0].x + A[i][0].y * A[i][0].y;
      C[0][1] += A[i][0].x * A[i][1].x + A[i][0].y * A[i][1].y;
      C[1][0] = C[0][1];
      C[1][1] += A[i][1].x * A[i][1].x + A[i][1].y * A[i][1].y;
      X[0] += A[i][0].x * tmp.x + A[i][0].y * tmp.y;
      X[1] += A[i][1].x * tmp.x + A[i][1].y * tmp.y;
    }

    const detC0C1 = C[0][0] * C[1][1] - C[1][0] * C[0][1];
    const detC0X = C[0][0] * X[1] - C[1][0] * X[0];
    const detXC1 = X[0] * C[1][1] - X[1] * C[0][1];
    let alphaL = detC0C1 == 0 ? 0 : detXC1 / detC0C1;
    let alphaR = detC0C1 == 0 ? 0 : detC0X / detC0C1;
    const segLength = CurveFitter.#dist(p0, p3);
    const epsilon = 1e-6 * segLength;

    if(alphaL < epsilon || alphaR < epsilon) {
      /* Wu/Barsky heuristic */
      alphaL = alphaR = segLength / 3;
    }

    return [p0, CurveFitter.#addScaled(p0, tHat1, alphaL), CurveFitter.#addScaled(p3, tHat2, alphaR), p3];
  }

  static #reparameterize(pts, first, last, u, bez) {
    const uPrime = [];

    for(let i = first; i <= last; i++) uPrime.push(CurveFitter.#newtonRaphson(bez, pts[i], u[i - first]));

    return uPrime;
  }

  static #newtonRaphson(bez, p, t) {
    const q = CurveFitter.evaluate(bez, t);
    const q1 = [],
      q2 = [];

    for(let i = 0; i < 3; i++)
      q1.push({
        x: (bez[i + 1].x - bez[i].x) * 3,
        y: (bez[i + 1].y - bez[i].y) * 3,
      });
    for(let i = 0; i < 2; i++)
      q2.push({
        x: (q1[i + 1].x - q1[i].x) * 2,
        y: (q1[i + 1].y - q1[i].y) * 2,
      });

    const d1 = CurveFitter.#bezierEval(q1, 2, t);
    const d2 = CurveFitter.#bezierEval(q2, 1, t);
    const numerator = (q.x - p.x) * d1.x + (q.y - p.y) * d1.y;
    const denominator = d1.x * d1.x + d1.y * d1.y + (q.x - p.x) * d2.x + (q.y - p.y) * d2.y;

    if(denominator == 0) return t;

    return t - numerator / denominator;
  }

  static #bezierEval(ctrl, degree, t) {
    const tmp = ctrl.map(p => ({ x: p.x, y: p.y }));

    for(let i = 1; i <= degree; i++)
      for(let j = 0; j <= degree - i; j++) {
        tmp[j].x = (1 - t) * tmp[j].x + t * tmp[j + 1].x;
        tmp[j].y = (1 - t) * tmp[j].y + t * tmp[j + 1].y;
      }

    return tmp[0];
  }

  static #chordLengthParameterize(pts, first, last) {
    const u = [0];

    for(let i = first + 1; i <= last; i++) u.push(u[u.length - 1] + CurveFitter.#dist(pts[i], pts[i - 1]));

    const total = u[u.length - 1] || 1;

    return u.map(v => v / total);
  }

  static #computeMaxError(pts, first, last, bez, u) {
    let maxError = 0,
      splitPoint = first + ((last - first) >> 1);

    for(let i = first + 1; i < last; i++) {
      const p = CurveFitter.evaluate(bez, u[i - first]);
      const dx = p.x - pts[i].x,
        dy = p.y - pts[i].y;
      const distSq = dx * dx + dy * dy;

      if(distSq > maxError) {
        maxError = distSq;
        splitPoint = i;
      }
    }

    return { maxError, splitPoint };
  }

  static #toPoints(xy) {
    const pts = [];

    for(let i = 0; i + 1 < xy.length; i += 2) pts.push({ x: xy[i], y: xy[i + 1] });

    return pts;
  }

  static #mapToIndices(pts, coords) {
    if(coords.length == 0) return [];

    const map = new Map();

    for(let i = 0; i < pts.length; i++) {
      const key = pts[i].x + ',' + pts[i].y;

      if(!map.has(key)) map.set(key, i);
    }

    const idx = new Set();

    for(let c of coords) {
      let i = map.get(c.x + ',' + c.y);

      if(i === undefined) {
        /* fallback: nearest raw point */
        let best = Infinity;

        for(let j = 0; j < pts.length; j++) {
          const d = CurveFitter.#dist(pts[j], c);

          if(d < best) {
            best = d;
            i = j;
          }
        }
      }

      idx.add(i);
    }

    return [...idx];
  }

  static #sliceWrap(pts, a, b) {
    const n = pts.length,
      run = [];
    let i = a;

    do {
      run.push(pts[i]);
      i = (i + 1) % n;
    } while(i != b);

    run.push(pts[b]);

    return run;
  }

  static #sub(a, b) {
    return { x: a.x - b.x, y: a.y - b.y };
  }

  static #addScaled(p, v, s) {
    return { x: p.x + v.x * s, y: p.y + v.y * s };
  }

  static #dist(a, b) {
    return Math.hypot(a.x - b.x, a.y - b.y);
  }

  static #normalize(v) {
    const l = Math.hypot(v.x, v.y);

    return l == 0 ? { x: 0, y: 0 } : { x: v.x / l, y: v.y / l };
  }
}

/* ------------------------------------------------------------------ *
 * SvgBuilder: regions -> SVG document
 * ------------------------------------------------------------------ */
export class SvgBuilder {
  /* width/height: original resolution; scale: divide coordinates by this */
  constructor(width, height, options = {}) {
    this.width = width;
    this.height = height;
    this.scale = options.scale ?? 1;
    this.strokeWidth = options.strokeWidth ?? defaultOptions.strokeWidth;
    this.regions = [];
  }

  /* rings: array of bezier segment lists; rings[0] is the outer boundary,
   * the rest are holes (rendered via fill-rule evenodd) */
  addRegion(color, rings, area) {
    this.regions.push({ color, rings, area });
  }

  build() {
    /* back-to-front: big regions first, detail on top */
    const sorted = [...this.regions].sort((a, b) => b.area - a.area);
    const lines = [`<svg xmlns="http://www.w3.org/2000/svg" width="${this.width}" height="${this.height}" viewBox="0 0 ${this.width} ${this.height}">`];

    for(let { color, rings } of sorted) {
      const d = rings
        .filter(segs => segs.length > 0)
        .map(segs => this.#ringToPath(segs))
        .join(' ');

      if(d == '') continue;

      lines.push(`<path d="${d}" fill="${color}" stroke="${color}" stroke-width="${this.strokeWidth}" fill-rule="evenodd"/>`);
    }

    lines.push('</svg>');

    return lines.join('\n');
  }

  #ringToPath(segs) {
    const c = v => {
      const r = Math.round((v / this.scale) * 100) / 100;

      if(!Number.isFinite(r)) throw new Error('SvgBuilder: non-finite coordinate in path');

      return r;
    };
    let d = `M${c(segs[0][0].x)} ${c(segs[0][0].y)}`;

    for(let [, c1, c2, p3] of segs) d += `C${c(c1.x)} ${c(c1.y)} ${c(c2.x)} ${c(c2.y)} ${c(p3.x)} ${c(p3.y)}`;

    return d + 'Z';
  }
}

/* ------------------------------------------------------------------ *
 * Vectorizer: facade tying the stages together
 * ------------------------------------------------------------------ */
export class Vectorizer {
  constructor(options = {}) {
    this.options = { ...defaultOptions, ...options };
    this.stages = {};
    this.layers = [];
  }

  /* vectorize(mat or path) -> SVG string */
  vectorize(input) {
    const opts = this.options;
    const mat = typeof input == 'string' ? imread(input) : input;

    if(!mat || mat.empty()) throw new Error(`Vectorizer: cannot read input ${typeof input == 'string' ? `'${input}'` : 'Mat'}`);

    const { cols: width, rows: height } = mat;
    let masks = null;

    if(opts.model) {
      const segmenter = this.#time('segment', () => new OnnxSegmenter(opts.model, opts));

      masks = this.#time('segment.forward', () => segmenter.segment(mat));
      this.#log(`segmentation: ${masks.length} mask(s)`);
    }

    const quantizer = new ColorQuantizer(opts.colors, opts);
    const { labelMap, palette, filtered } = this.#time('quantize', () => quantizer.quantize(mat));

    this.stages = {
      input: typeof input == 'string' ? null : mat,
      filtered,
      labelMap,
      palette,
    };
    this.#log(`palette: ${palette.map(hexColor).join(' ')}`);

    const tracer = new ContourTracer(opts);
    const traced = this.#time('trace', () => tracer.trace(labelMap, palette.length, masks));

    if(masks) for(let m of masks) m.release();

    const simplifier = new PolySimplifier(opts.simplify);
    const fitter = new CurveFitter(opts.tolerance, opts.cornerThreshold);
    const builder = new SvgBuilder(width, height, {
      scale: opts.upscale,
      strokeWidth: opts.strokeWidth,
    });
    let nRegions = 0,
      nSegments = 0;

    this.layers = [];
    this.#time('fit', () => {
      for(let { colorIndex, regions } of traced) {
        const color = hexColor(palette[colorIndex]);
        const layer = {
          colorIndex,
          color,
          bgr: palette[colorIndex],
          regions: [],
        };

        for(let { outer, holes, area } of regions) {
          const rings = [];

          for(let contour of [outer, ...holes]) {
            const clean = contour.simplifyRadialDistance(opts.radialTolerance);
            const simplified = simplifier.simplify(clean);
            const segs = fitter.fitClosed(clean.array, simplified.array);

            if(segs.length > 0) rings.push(segs);

            nSegments += segs.length;
          }

          if(rings.length == 0) continue;

          builder.addRegion(color, rings, area);
          layer.regions.push({ area, outer, holes, rings });
          nRegions++;
        }

        this.layers.push(layer);
      }
    });
    this.#log(`fit: ${nRegions} regions, ${nSegments} bezier segments`);

    const svg = this.#time('svg', () => builder.build());

    if(typeof input == 'string') mat.release();

    return svg;
  }

  /* per-color layer info from the last vectorize() call, for debugging */
  getLayers() {
    return this.layers;
  }

  #log(msg) {
    if(this.options.verbose) console.log(`[vectorize] ${msg}`);
  }

  #time(name, fn) {
    const start = Date.now();
    const result = fn();

    this.#log(`${name}: ${Date.now() - start} ms`);

    return result;
  }
}

/* ------------------------------------------------------------------ *
 * Playground: pluggable conditioning / vectorization / post-processing
 * processors, decomposed out of the monolithic vectorizer/vector/methods/*
 * strategies so they can be freely recombined (e.g. swap Canny for a DNN
 * edge model upstream of the same line tracer).
 *
 * Conditioning stages are cv.Mat -> cv.Mat and run through cvPipeline's
 * `Processor` (Mat-reuse + `.watch(params)` dirty tracking, see
 * cvPipeline.js). Vectorization/post stages produce VectorData, not Mats,
 * so they use the lighter `Stage` cache below instead of forcing a Mat
 * allocation on data that isn't image-shaped — same dirty-flag contract
 * (`.watch(...)`, recompute cascades downstream once anything upstream
 * actually reran), just without cvPipeline's Mat-reuse mapper.
 * ------------------------------------------------------------------ */

/* A memoized pipeline node for stages that don't output a Mat (contours,
 * VectorData, ...). `forced=true` means an upstream stage already reran this
 * call, so this one must too regardless of its own params. */
class Stage {
  constructor(id, fn, params = []) {
    this.id = id;
    this.fn = fn;
    this.params = params;
    this.dirty = true;
    this.cached = undefined;
  }

  get isDirty() {
    return this.dirty || this.params.some(p => p && p.dirty);
  }

  run(input, forced = false) {
    if(forced || this.isDirty || this.cached === undefined) {
      this.cached = this.fn(input, this.params);
      this.dirty = false;
      for(const p of this.params) if(p) p.dirty = false;
      return { value: this.cached, recomputed: true };
    }
    return { value: this.cached, recomputed: false };
  }
}

/* --- conditioning (Mat -> Mat), cvPipeline Processors --- */

export const ConditioningProcessors = {
  /* Plain grayscale - the baseline everything else compares against. */
  grayscale() {
    return Processor(function grayscale(src, dst) {
      if(src.channels() === 1) src.copyTo(dst);
      else cvtColor(src, dst, COLOR_BGR2GRAY);
    });
  },

  /* CLAHE local-contrast equalization - helps uneven book-page lighting.
   * Expects a single-channel input (chain after grayscale()). */
  clahe() {
    const clipLimit = new NumericParam(2, 1, 40, 0.5);
    const tileSize = new NumericParam(8, 2, 32, 1);
    let clahe = null,
      lastTile = -1;

    const proc = Processor(function clahe_(src, dst) {
      if(!clahe || lastTile !== tileSize.get()) {
        lastTile = tileSize.get();
        clahe = createCLAHE(clipLimit.get(), new Size(lastTile, lastTile));
      } else {
        clahe.clipLimit = clipLimit.get();
      }
      clahe.apply(src, dst);
    });

    return proc.watch(clipLimit, tileSize);
  },

  /* Gaussian blur - cheap denoise before edge extraction. */
  blur() {
    const ksize = new NumericParam(3, 0, 15, 2);

    const proc = Processor(function blur(src, dst) {
      const k = ksize.get();
      if(k < 3) src.copyTo(dst);
      else GaussianBlur(src, dst, new Size(k | 1, k | 1), 0);
    });

    return proc.watch(ksize);
  },

  /* DexiNed learned edge detector - semantic edges, no texture noise.
   * Model: examples/models/edge_detection_dexined/edge_detection_dexined_2024sep.onnx */
  dexined(modelPath = 'examples/models/edge_detection_dexined/edge_detection_dexined_2024sep.onnx') {
    let net = null;

    return Processor(function dexined_(src, dst) {
      net ??= readNetFromONNX(modelPath);

      const size = new Size(512, 512);
      const blob = new Mat();
      blobFromImage(src, blob, 1.0, size, [103.939, 116.779, 123.68, 0], false, false);
      net.setInput(blob);

      const out = net.forward();
      const data = new Float32Array(out.buffer);
      let min = Infinity,
        max = -Infinity;
      for(const v of data) {
        if(v < min) min = v;
        if(v > max) max = v;
      }
      const range = max - min || 1;
      const small = new Mat(size, CV_8UC1);
      const u8 = new Uint8Array(small.buffer);
      for(let i = 0; i < data.length; i++) u8[i] = Math.round(((data[i] - min) / range) * 255);

      resize(small, dst, src.size(), 0, 0, INTER_NEAREST);
      blob.delete?.();
      out.delete?.();
      small.delete?.();
    });
  },

  /* Structured (Dollar/Zitnick) edge forest - classic ML, not deep, often
   * cleaner than Canny on line-art/scans. Needs tests/model.yml.gz. */
  structuredEdges(modelPath = 'tests/model.yml.gz') {
    let detector = null;

    return Processor(function structuredEdges_(src, dst) {
      detector ??= ximgproc.createStructuredEdgeDetection(modelPath);

      const floatSrc = new Mat();
      src.convertTo(floatSrc, CV_32FC3, 1 / 255.0);

      const edges = new Mat();
      detector.detectEdges(floatSrc, edges);
      edges.convertTo(dst, CV_8UC1, 255.0);

      floatSrc.delete?.();
      edges.delete?.();
    });
  },
};

/* --- vectorization (Mat -> VectorData), Stage --- */

export const VectorizationStages = {
  /* The baseline: Canny -> findContours -> stroked polylines. */
  cannyContours() {
    const thresh1 = new NumericParam(50, 0, 255, 1);
    const thresh2 = new NumericParam(150, 0, 255, 1);

    return new Stage(
      'cannyContours',
      mat => {
        const edges = new Mat();
        Canny(mat, edges, thresh1.get(), thresh2.get());
        const contours = new MatVector(),
          hierarchy = [];
        findContours(edges, contours, hierarchy, RETR_LIST, CHAIN_APPROX_SIMPLE);
        const shapes = contoursToShapes(contours, { mode: 'stroke', minPoints: 2 });
        edges.delete?.();
        return createVectorData(mat.cols, mat.rows, { shapes });
      },
      [thresh1, thresh2],
    );
  },

  /* EdgeDrawing (EDPF): parametric line tracing, not a contour-around-edges
   * approximation - the closer match for straight schematic wires/borders. */
  edgeDrawingLines() {
    let ed = null;

    return new Stage('edgeDrawingLines', mat => {
      ed ??= ximgproc.createEdgeDrawing();
      const lines = new Mat();
      ed.detectEdges(mat);
      ed.detectLines(lines);
      const shapes = linesToShapes(lines);
      lines.delete?.();
      return createVectorData(mat.cols, mat.rows, { shapes });
    });
  },

  /* Topology-aware skeleton tracing (algorithms/skeleton_lines.hpp): needs a
   * binary (thinned or not) input; cuts polylines at junctions rather than
   * walking through them, unlike a plain contour trace. */
  skeletonTrace() {
    return new Stage('skeletonTrace', mat => {
      const pvv = new PointVectorVector();
      traceSkeleton(mat, pvv);
      const shapes = [];
      for(const pts of pvv) {
        const flat = [];
        for(const p of pts) flat.push([p.x ?? p[0], p.y ?? p[1]]);
        if(flat.length >= 2) shapes.push({ kind: 'polyline', points: flat, style: { stroke: '#101010', strokeWidth: 1, fill: null } });
      }
      return createVectorData(mat.cols, mat.rows, { shapes });
    });
  },
};

/* --- post-processing (VectorData -> VectorData), Stage --- */

export const PostProcessingStages = {
  /* approxPolyDP per shape. */
  approxPoly() {
    const epsilon = new NumericParam(1.5, 0.1, 20, 0.1);

    return new Stage(
      'approxPoly',
      vd => {
        const shapes = vd.shapes.map(sh => {
          if(sh.kind !== 'polyline' && sh.kind !== 'polygon') return sh;
          const flat = new Mat(new Size(1, sh.points.length), CV_32SC2);
          const d = new Int32Array(flat.buffer);
          sh.points.forEach(([x, y], i) => {
            d[i * 2] = Math.round(x);
            d[i * 2 + 1] = Math.round(y);
          });
          const approx = new Mat();
          approxPolyDP(flat, approx, epsilon.get(), sh.kind === 'polygon');
          const out = Array.from(new Int32Array(approx.buffer));
          const points = [];
          for(let i = 0; i + 1 < out.length; i += 2) points.push([out[i], out[i + 1]]);
          flat.delete?.();
          approx.delete?.();
          return { ...sh, points };
        });
        return { ...vd, shapes };
      },
      [epsilon],
    );
  },

  /* psimpl polyline simplification (Reumann-Witkam by default). */
  psimplSimplify(method = 'douglasPeucker') {
    const tolerance = new NumericParam(1, 0.1, 20, 0.1);
    const fn = psimpl[method];
    if(typeof fn !== 'function') throw new Error(`PostProcessingStages.psimplSimplify: unknown method '${method}'`);

    return new Stage(
      `psimpl:${method}`,
      vd => {
        const shapes = vd.shapes.map(sh => {
          if(sh.kind !== 'polyline' && sh.kind !== 'polygon') return sh;
          const out = fn(sh.points, tolerance.get());
          const d = new Int32Array(out.buffer);
          const points = [];
          for(let i = 0; i + 1 < d.length; i += 2) points.push([d[i], d[i + 1]]);
          out.delete?.();
          return { ...sh, points };
        });
        return { ...vd, shapes };
      },
      [tolerance],
    );
  },
};

/* Composes conditioning Processors (Mat->Mat, cvPipeline-style) with one
 * vectorization Stage and a chain of post-processing Stages. Only the
 * stages whose own params changed (plus everything after the earliest such
 * stage) get recomputed on `.run()` - see cvPipeline.js's `.watch()`. */
export class VectorizationPipeline {
  constructor({ conditioning = [], vectorize, post = [] }) {
    this.conditioning = conditioning;
    this.vectorize = vectorize;
    this.post = post;
  }

  run(srcMat) {
    let mat = srcMat;
    let force = false;

    for(const proc of this.conditioning) {
      const mustRun = !proc.managed || force || proc.isDirty;
      if(mustRun) {
        mat = proc(mat, proc.out);
        if(proc.managed) proc.clean();
        force = true;
      } else {
        mat = proc.out;
      }
    }

    const vecResult = this.vectorize.run(mat, force);
    force = force || vecResult.recomputed;
    let data = vecResult.value;

    for(const stage of this.post) {
      const r = stage.run(data, force);
      data = r.value;
      force = force || r.recomputed;
    }

    return data;
  }
}

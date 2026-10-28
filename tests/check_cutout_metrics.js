import * as cv from 'opencv';
import process from 'process';
/*
 * Acceptance metrics of doc/opencv-imgproc.md S14.7 for one cutout. Run the
 * tool without --trim, with --rest-out and no --rest-fill, then
 *   QUICKJS_MODULE_PATH=<build> qjsm tests/check_cutout_metrics.js photo.jpg cut.png rest.png
 * prints one JSON line: composite (solid interior and rest must equal the
 * photo, the edge band shows how far the decontamination moved the colour),
 * fringe (band pixels whose colour is nearer the local background than the
 * local foreground, with and without the decontaminated colour) and border
 * (opaque 12 px inside the frame that is not opaque on the frame).
 */
const [origFile, cutFile, restFile] = process.argv.slice(2);
if(!restFile) { console.log(JSON.stringify({ error: 'usage: check_cutout_metrics.js photo cut.png rest.png' })); process.exit(2); }
const orig = cv.imread(origFile);
const cutA = cv.imread(cutFile, cv.IMREAD_UNCHANGED);
const restA = cv.imread(restFile, cv.IMREAD_UNCHANGED);
const out = { photo: orig.cols + 'x' + orig.rows };
if(cutA.cols != orig.cols || cutA.rows != orig.rows) { console.log(JSON.stringify({ error: 'size mismatch', cut: [cutA.cols, cutA.rows], orig: [orig.cols, orig.rows] })); process.exit(1); }

const [cb, cg, cr, ca] = cv.split(cutA);
const cutRGB = new cv.Mat(); cv.merge([cb, cg, cr], cutRGB);
const [rb, rg, rr, ra] = cv.split(restA);
const restRGB = new cv.Mat(); cv.merge([rb, rg, rr], restRGB);
const total = orig.cols * orig.rows;
const maxCh = d => { const [x, y, z] = cv.split(d); const m = new cv.Mat(); cv.max(x, y, m); cv.max(m, z, m); return m; };

// 1. composite back: where alpha = 1 the cut must equal the photo, where the
// cut has no alpha the rest must; the edge band shows what the decontamination moved
const dCut = maxCh((() => { const d = new cv.Mat(); cv.absdiff(cutRGB, orig, d); return d; })());
const dRest = maxCh((() => { const d = new cv.Mat(); cv.absdiff(restRGB, orig, d); return d; })());
const sel = (lo, hi, a) => { const m = new cv.Mat(), m2 = new cv.Mat(); cv.compare(a, new cv.Scalar(lo - 1), m, cv.CMP_GT); cv.compare(a, new cv.Scalar(hi + 1), m2, cv.CMP_LT); cv.bitwise_and(m, m2, m); return m; };
const stats = (d, m) => { const n = cv.countNonZero(m); if(!n) return { n: 0 }; const mean = cv.mean(d, m)[0]; const hot = new cv.Mat(); cv.compare(d, new cv.Scalar(8), hot, cv.CMP_GT); cv.bitwise_and(hot, m, hot); return { n, meanDiff: +mean.toFixed(2), over8: +(cv.countNonZero(hot) / n).toFixed(4) }; };
out.composite = {
  cutSolid_a255: stats(dCut, sel(255, 255, ca)),
  cutBand_a1_254: stats(dCut, sel(1, 254, ca)),
  restOutside_cutA0: stats(dRest, sel(0, 0, ca)),
  restBand_cutA1_254: stats(dRest, sel(1, 254, ca)),
};

// 2. fringe: band pixels whose foreground colour is nearer the local background
// than the local foreground. same count for the raw photo colour (no decontamination)
const long = Math.max(orig.cols, orig.rows), s = Math.min(1, 1600 / long);
const small = m => { if(s >= 1) return m; const r = new cv.Mat(); cv.resize(m, r, new cv.Size(Math.round(m.cols * s), Math.round(m.rows * s)), 0, 0, cv.INTER_AREA); return r; };
const f32 = (m, k = 1 / 255) => { const f = new cv.Mat(); m.convertTo(f, cv.CV_32F, k); return f; };
const O = f32(small(orig)), F = f32(small(cutRGB)), A = f32(small(ca));
const W = A.cols, H = A.rows;
const rad = Math.max(3, Math.round(0.008 * Math.max(W, H)));
const ksz = new cv.Size(2 * rad + 1, 2 * rad + 1);
const thr = (m, op, v) => { const o = new cv.Mat(); cv.threshold(m, o, v, 1, op); return o; };
const solid = thr(A, cv.THRESH_BINARY, 0.98);
const clear = (() => { const o = new cv.Mat(); cv.threshold(A, o, 0.02, 1, cv.THRESH_BINARY_INV); return o; })();
const blur = m => { const o = new cv.Mat(); cv.blur(m, o, ksz); return o; };
const wsum = (img, w) => { const chans = cv.split(img).map(c => { const m = new cv.Mat(); cv.multiply(c, w, m); return blur(m); }); const bw = blur(w); const eps = new cv.Mat(H, W, cv.CV_32F, new cv.Scalar(1e-6)); cv.add(bw, eps, bw); return { c: chans.map(c => { const o = new cv.Mat(); cv.divide(c, bw, o); return o; }), w: bw }; };
const fgL = wsum(O, solid), bgL = wsum(O, clear);
const band = (() => { const lo = thr(A, cv.THRESH_BINARY, 0.04), hi = (() => { const o = new cv.Mat(); cv.threshold(A, o, 0.96, 1, cv.THRESH_BINARY_INV); return o; })(); const o = new cv.Mat(); cv.multiply(lo, hi, o); return o; })();
const valid = (() => { const a = thr(fgL.w, cv.THRESH_BINARY, 0.05), b = thr(bgL.w, cv.THRESH_BINARY, 0.05); const o = new cv.Mat(); cv.multiply(a, b, o); cv.multiply(o, band, o); return o; })();
const dist = (img, ref) => { const acc = new cv.Mat(H, W, cv.CV_32F, new cv.Scalar(0)); cv.split(img).forEach((c, i) => { const d = new cv.Mat(); cv.subtract(c, ref.c[i], d); cv.multiply(d, d, d); cv.add(acc, d, acc); }); cv.sqrt(acc, acc); return acc; };
const dFB = (() => { const acc = new cv.Mat(H, W, cv.CV_32F, new cv.Scalar(0)); fgL.c.forEach((c, i) => { const d = new cv.Mat(); cv.subtract(c, bgL.c[i], d); cv.multiply(d, d, d); cv.add(acc, d, acc); }); cv.sqrt(acc, acc); return acc; })();
const fringeOf = img => {
  const dB = dist(img, bgL), dF = dist(img, fgL);
  const half = new cv.Mat(); dFB.convertTo(half, cv.CV_32F, 0.5);
  const nearBg = new cv.Mat(), farFg = new cv.Mat(), sepOK = new cv.Mat();
  cv.compare(dB, half, nearBg, cv.CMP_LT); cv.compare(dF, half, farFg, cv.CMP_GT); cv.compare(dFB, new cv.Scalar(0.12), sepOK, cv.CMP_GT);
  const v = new cv.Mat(); valid.convertTo(v, cv.CV_8U, 255);
  const hit = new cv.Mat(); cv.bitwise_and(nearBg, farFg, hit); cv.bitwise_and(hit, sepOK, hit); cv.bitwise_and(hit, v, hit);
  const nBand = cv.countNonZero(v);
  return { bandPixels: nBand, fringePixels: cv.countNonZero(hit), fringeFraction: nBand ? +(cv.countNonZero(hit) / nBand).toFixed(4) : 0 };
};
out.fringe = { scale: +s.toFixed(3), radiusPx: rad, decontaminated: fringeOf(F), rawPhotoColour: fringeOf(O) };

// 3. subject at the image border: opaque 12 px inside must stay opaque on the edge
const ins = 12, border = {};
const side = (name, edge, inner) => { const e = edge, i = inner; const opaqueIn = new cv.Mat(), edgeLow = new cv.Mat(); cv.compare(i, new cv.Scalar(250), opaqueIn, cv.CMP_GT); cv.compare(e, new cv.Scalar(229), edgeLow, cv.CMP_LT); const lost = new cv.Mat(); cv.bitwise_and(opaqueIn, edgeLow, lost); const n = cv.countNonZero(opaqueIn); border[name] = { opaqueInside: n, lostAtEdge: cv.countNonZero(lost), lostFraction: n ? +(cv.countNonZero(lost) / n).toFixed(4) : 0 }; };
const w = ca.cols, h = ca.rows;
side('left', ca.roi(new cv.Rect(0, 0, 1, h)), ca.roi(new cv.Rect(ins, 0, 1, h)));
side('right', ca.roi(new cv.Rect(w - 1, 0, 1, h)), ca.roi(new cv.Rect(w - 1 - ins, 0, 1, h)));
side('top', ca.roi(new cv.Rect(0, 0, w, 1)), ca.roi(new cv.Rect(0, ins, w, 1)));
side('bottom', ca.roi(new cv.Rect(0, h - 1, w, 1)), ca.roi(new cv.Rect(0, h - 1 - ins, w, 1)));
out.border = border;
console.log(JSON.stringify(out));

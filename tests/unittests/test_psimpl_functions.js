import { tests } from './tinytest.js';
import * as cv from 'opencv';

// Exercises every cv.psimpl.* polyline-simplification algorithm
// (douglasPeucker, reumannWitkam, radialDistance, nthPoint, opheim, lang,
// perpendicularDistance) against every argument type js_psimpl.cpp accepts
// (Mat CV_32SC2, Mat CV_64FC2, cv.PointVector, cv.Point2fVector, plain JS
// array), and checks that each returns the *same type it was given*
// (mirroring js_psimpl.cpp's documented contract - a plain array has no
// matching C++ container, so it falls back to Mat CV_32SC2), with
// "reasonable" output: strictly fewer points than the input, the
// polyline's first and last point always exactly preserved, and every
// returned point being one of the original vertices (all 7 algorithms are
// vertex-selection routines - none of them interpolate new points).

// ----- Shared polyline: a gentle sine wave, dense enough (dx=2 between
// x-steps) that the distance-based routines (radialDistance,
// reumannWitkam, opheim, lang, perpendicularDistance) genuinely get to
// merge adjacent points at the tolerances used below, while curving
// enough that the perpendicular/collinearity-based routines
// (douglasPeucker included) have real redundant points to remove. Chosen
// and verified empirically against the actual algorithms - see BUGS's
// psimpl-test-file-wrong-expectations entry for what goes wrong when a
// tolerance is picked without checking it against the polyline's own
// point spacing.

function buildIntPoints(n) {
  const pts = [];
  for (let i = 0; i < n; i++)
    pts.push({ x: Math.round(i * 2), y: Math.round(50 + 3 * Math.sin(i / 6)) });
  return pts;
}

function buildFloatPoints(n) {
  const pts = [];
  for (let i = 0; i < n; i++)
    pts.push({ x: i * 2, y: 50 + 3 * Math.sin(i / 6) });
  return pts;
}

const N = 40;
const INT_POINTS = buildIntPoints(N);
const FLOAT_POINTS = buildFloatPoints(N);

// ----- One builder per accepted cv.psimpl.* argument type -----

function matFromPoints32S(pts) {
  const m = new cv.Mat(pts.length, 1, cv.CV_32SC2);
  pts.forEach((p, i) => { m.data32S[i * 2] = p.x; m.data32S[i * 2 + 1] = p.y; });
  return m;
}

function matFromPoints64F(pts) {
  const m = new cv.Mat(pts.length, 1, cv.CV_64FC2);
  pts.forEach((p, i) => { m.data64F[i * 2] = p.x; m.data64F[i * 2 + 1] = p.y; });
  return m;
}

function pointVectorFromPoints(pts) {
  const v = new cv.PointVector();
  pts.forEach(p => v.push_back({ x: p.x, y: p.y }));
  return v;
}

function point2fVectorFromPoints(pts) {
  const v = new cv.Point2fVector();
  pts.forEach(p => v.push_back({ x: p.x, y: p.y }));
  return v;
}

// ----- Result introspection, generic across every returned type -----

function countOf(r) {
  return r instanceof cv.Mat ? r.rows : r.size();
}

function pointAt(r, i) {
  if (r instanceof cv.Mat) {
    return r.type() === cv.CV_32SC2
      ? { x: r.data32S[i * 2], y: r.data32S[i * 2 + 1] }
      : { x: r.data64F[i * 2], y: r.data64F[i * 2 + 1] };
  }
  const p = r.get(i);
  return { x: p.x, y: p.y };
}

function approxEq(a, b, eps) {
  return Math.abs(a - b) <= eps;
}

// Every accepted argument shape, with the expected result class (mirrors
// the input type; a plain array falls back to Mat CV_32SC2) and the
// comparison tolerance against the source points (0 for int/double
// storage, which round-trips exactly; a small epsilon for Point2fVector,
// whose cv::Point2f storage truncates to float32).
const INPUT_KINDS = [
  { label: 'Mat CV_32SC2', build: matFromPoints32S, source: INT_POINTS, resultClass: cv.Mat, eps: 0 },
  { label: 'Mat CV_64FC2', build: matFromPoints64F, source: FLOAT_POINTS, resultClass: cv.Mat, eps: 0 },
  { label: 'PointVector', build: pointVectorFromPoints, source: INT_POINTS, resultClass: cv.PointVector, eps: 0 },
  { label: 'Point2fVector', build: point2fVectorFromPoints, source: FLOAT_POINTS, resultClass: cv.Point2fVector, eps: 1e-2 },
  { label: 'JS array', build: pts => pts, source: INT_POINTS, resultClass: cv.Mat, eps: 0 },
];

// Every tolerance-based algorithm, with arguments tuned against
// INT_POINTS/FLOAT_POINTS above so every input type actually gets
// simplified (nthPoint is handled separately below - it takes an integer
// step, not a tolerance, and has a fully deterministic expected result).
const TOLERANCE_METHODS = [
  { name: 'douglasPeucker', args: [3.0] },
  { name: 'reumannWitkam', args: [3.0] },
  { name: 'radialDistance', args: [3.0] },
  { name: 'opheim', args: [2.0, 10.0] },
  { name: 'lang', args: [2.0, 10.0] },
  { name: 'perpendicularDistance', args: [3.0, 1] },
];

const testSuite = {};

for (const { name, args } of TOLERANCE_METHODS) {
  for (const { label, build, source, resultClass, eps } of INPUT_KINDS) {
    testSuite[`psimpl.${name} - ${label}`] = () => {
      const input = build(source);
      const result = cv.psimpl[name](input, ...args);

      if (!(result instanceof resultClass))
        throw new Error(`Expected a ${resultClass.name} back, got ${result}`);

      const count = countOf(result);
      if (count >= source.length)
        throw new Error(`Expected fewer points, got ${count} >= ${source.length}`);
      if (count < 2)
        throw new Error(`Expected at least the 2 endpoints, got ${count}`);

      const first = pointAt(result, 0);
      if (!approxEq(first.x, source[0].x, eps) || !approxEq(first.y, source[0].y, eps))
        throw new Error(`First point not preserved: got ${JSON.stringify(first)}, expected ${JSON.stringify(source[0])}`);

      const srcLast = source[source.length - 1];
      const last = pointAt(result, count - 1);
      if (!approxEq(last.x, srcLast.x, eps) || !approxEq(last.y, srcLast.y, eps))
        throw new Error(`Last point not preserved: got ${JSON.stringify(last)}, expected ${JSON.stringify(srcLast)}`);

      for (let i = 0; i < count; i++) {
        const p = pointAt(result, i);
        const matches = source.some(s => approxEq(p.x, s.x, eps) && approxEq(p.y, s.y, eps));
        if (!matches)
          throw new Error(`Point ${i} (${JSON.stringify(p)}) is not one of the original vertices`);
      }
    };
  }
}

// psimpl always keeps every nth point plus the polyline's own last point
// (see BUGS's psimpl-test-file-wrong-expectations, verified directly
// against include/psimpl.hpp) - so the exact result is predictable.
function expectedNthPointIndices(count, n) {
  const idx = [];
  for (let i = 0; i < count; i += n) idx.push(i);
  if (idx[idx.length - 1] !== count - 1) idx.push(count - 1);
  return idx;
}

const NTH_POINT_STEP = 3;

for (const { label, build, source, resultClass, eps } of INPUT_KINDS) {
  testSuite[`psimpl.nthPoint - ${label}`] = () => {
    const input = build(source);
    const result = cv.psimpl.nthPoint(input, NTH_POINT_STEP);

    if (!(result instanceof resultClass))
      throw new Error(`Expected a ${resultClass.name} back, got ${result}`);

    const expectedIdx = expectedNthPointIndices(source.length, NTH_POINT_STEP);
    const count = countOf(result);
    if (count !== expectedIdx.length)
      throw new Error(`Expected ${expectedIdx.length} points (every ${NTH_POINT_STEP}th plus the last), got ${count}`);

    for (let i = 0; i < count; i++) {
      const p = pointAt(result, i);
      const expected = source[expectedIdx[i]];
      if (!approxEq(p.x, expected.x, eps) || !approxEq(p.y, expected.y, eps))
        throw new Error(`Point ${i}: got ${JSON.stringify(p)}, expected ${JSON.stringify(expected)} (source index ${expectedIdx[i]})`);
    }
  };
}

// ----- Argument-type validation -----

testSuite['psimpl.douglasPeucker - rejects an unsupported input type'] = () => {
  let threw = false;
  try {
    cv.psimpl.douglasPeucker(42, 5.0);
  } catch (e) {
    threw = true;
  }
  if (!threw)
    throw new Error('Expected a TypeError for an unsupported input type');
};

testSuite['psimpl.douglasPeucker - rejects a Mat of the wrong element type'] = () => {
  const m = new cv.Mat(4, 1, cv.CV_8UC1);
  let threw = false;
  try {
    cv.psimpl.douglasPeucker(m, 5.0);
  } catch (e) {
    threw = true;
  }
  if (!threw)
    throw new Error('Expected a TypeError for a Mat of the wrong element type');
};

tests(testSuite);

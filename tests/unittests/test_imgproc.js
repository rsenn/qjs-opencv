import { tests, eq, assert } from './tinytest.js';
import * as cv from 'opencv';

/*
 * Exercises the shape-analysis methods implemented by js_imgproc_shape()
 * (js_imgproc.cpp: approxPolyDP, arcLength, connectedComponents,
 * connectedComponentsWithStats, contourArea, convexHull, convexityDefects,
 * fitEllipse, fitEllipseAMS, fitEllipseDirect, fitLine, intersectConvexConvex,
 * isContourConvex, matchShapes, minAreaRect, minEnclosingCircle,
 * minEnclosingTriangle) against real contours: draw a filled rectangle and a
 * filled circle into a Mat, then run cv.findContours() twice - once
 * collecting plain cv.Contour objects into a JS array ("Contours" mode),
 * once into a cv.PointVectorVector - and use both to test each method.
 */

const IMG_SIZE = 240;
const RECT = { x: 20, y: 20, width: 80, height: 80 };
const RECT_AREA = RECT.width * RECT.height;
const CIRCLE_CENTER = { x: 170, y: 170 };
const CIRCLE_RADIUS = 40;

function drawShapesImage() {
  const img = cv.Mat.zeros(IMG_SIZE, IMG_SIZE, cv.CV_8UC1);
  cv.rectangle(img, RECT, 255, -1);
  cv.circle(img, CIRCLE_CENTER, CIRCLE_RADIUS, 255, -1);
  return img;
}

// findContours(image, [], hierarchy) fills the array with cv.Mat CV_32SC2
// instances ("Contours" mode).
function findContoursAsContours(img) {
  const contours = [];
  const hierarchy = [];
  cv.findContours(img, contours, hierarchy, cv.RETR_EXTERNAL, cv.CHAIN_APPROX_SIMPLE);
  return contours;
}

// findContours(image, PointVectorVector, hierarchy) fills nested
// std::vector<cv::Point> per contour, zero-copy on the C++ side.
// PointVectorVector.get(i) hands back a genuine PointVector instance.
function findContoursAsPointVectors(img) {
  const pvv = new cv.PointVectorVector();
  const hierarchy = new cv.Mat();
  cv.findContours(img, pvv, hierarchy, cv.RETR_EXTERNAL, cv.CHAIN_APPROX_SIMPLE);

  const result = [];
  for (let i = 0; i < pvv.size(); i++) result.push(pvv.get(i));
  return result;
}

// The circle's boundary keeps far more points after CHAIN_APPROX_SIMPLE than
// the rectangle's mostly-straight edges, so point count reliably tells the
// two apart regardless of findContours' discovery order.
function isCircleLike(pointCount) {
  return pointCount > 50;
}

function classify(items, lengthOf) {
  let rect, circle;

  for (const item of items) {
    if (isCircleLike(lengthOf(item))) circle = item;
    else rect = item;
  }

  if (!rect || !circle) throw new Error('expected to find both a rectangle and a circle contour');

  return { rect, circle };
}

const img = drawShapesImage();

const contoursFromArray = findContoursAsContours(img);
const arrayShapes = classify(contoursFromArray, mat => mat.rows);
const rectMat = arrayShapes.rect;
const circleMat = arrayShapes.circle;

const contoursFromPVV = findContoursAsPointVectors(img);
const pvvShapes = classify(contoursFromPVV, pv => pv.size());
const rectPV = pvvShapes.rect;
const circlePV = pvvShapes.circle;

// Every per-contour test below runs once per contour representation.
const rectByRepresentation = [
  ['Mat (from findContours)', rectMat],
  ['PointVector', rectPV],
];
const circleByRepresentation = [
  ['Mat (from findContours)', circleMat],
  ['PointVector', circlePV],
];

const testCases = {
  'findContours - Contours (array) mode finds both shapes'() {
    eq(2, contoursFromArray.length);
  },

  'findContours - PointVectorVector mode finds both shapes'() {
    eq(2, contoursFromPVV.length);
  },

  // connectedComponents/connectedComponentsWithStats label the source image
  // directly - they don't take a contour argument, so (unlike the rest of
  // this file) there's nothing to repeat per contour representation.
  'connectedComponents - labels rectangle and circle'() {
    const labels = new cv.Mat();
    const n = cv.connectedComponents(img, labels, 8, cv.CV_32S, cv.CCL_DEFAULT);

    eq(3, n); // background + rectangle + circle
    eq(img.rows, labels.rows);
    eq(img.cols, labels.cols);
  },

  'connectedComponentsWithStats - stats and centroids for rectangle and circle'() {
    const labels = new cv.Mat();
    const stats = new cv.Mat();
    const centroids = new cv.Mat();
    const n = cv.connectedComponentsWithStats(img, labels, stats, centroids, 8, cv.CV_32S, cv.CCL_DEFAULT);

    eq(3, n);
    eq(n, stats.rows);
    eq(5, stats.cols); // x, y, width, height, area
    eq(n, centroids.rows);
    eq(2, centroids.cols);
  },
};

function addTest(name, fn) {
  if (testCases[name]) throw new Error(`duplicate test name: ${name}`);
  testCases[name] = fn;
}

for (const [label, rect] of rectByRepresentation) {
  addTest(`contourArea - rectangle (${label})`, () => {
    console.log('rect',rect);
    const area = cv.contourArea(rect);
    assert(Math.abs(area - RECT_AREA) < RECT_AREA * 0.1, `expected area ~${RECT_AREA}, got ${area}`);
  });

  addTest(`arcLength - rectangle (${label})`, () => {
    const length = cv.arcLength(rect, true);
    const expected = 4 * RECT.width;
    assert(Math.abs(length - expected) < expected * 0.1, `expected length ~${expected}, got ${length}`);
  });

  addTest(`isContourConvex - rectangle (${label})`, () => {
    eq(true, cv.isContourConvex(rect));
  });

  addTest(`convexHull - rectangle (${label})`, () => {
    const hull = new cv.Mat();
    cv.convexHull(rect, hull);
    assert(hull.total() >= 4, `expected at least 4 hull points, got ${hull.total()}`);
    eq(cv.CV_32SC2, hull.type());
  });

  addTest(`convexityDefects - rectangle (${label})`, () => {
    const hullIdx = new cv.Mat();
    cv.convexHull(rect, hullIdx, false, false); // returnPoints=false -> hull of indices, as convexityDefects requires
    const defects = new cv.Mat();
    cv.convexityDefects(rect, hullIdx, defects);
    // a perfect rectangle has no convexity defects; only check the shape when some were found
    if(defects.total() > 0) {
      eq(4, defects.cols); // [start, end, farthest, fixpt_depth] per defect - opencv.js convention
      eq(1, defects.channels());
    }
  });

  addTest(`approxPolyDP - rectangle (${label})`, () => {
    const approx = new cv.Mat();
    cv.approxPolyDP(rect, approx, 3, true);
    assert(approx.total() >= 4 && approx.total() <= 8, `expected ~4 corners, got ${approx.total()}`);
    eq(cv.CV_32SC2, approx.type());
  });

  addTest(`minAreaRect - rectangle (${label})`, () => {
    const rr = cv.minAreaRect(rect);
    assert(Math.abs(rr.center.x - 60) < 5, `expected center.x ~60, got ${rr.center.x}`);
    assert(Math.abs(rr.center.y - 60) < 5, `expected center.y ~60, got ${rr.center.y}`);
  });

  addTest(`minEnclosingCircle - rectangle (${label})`, () => {
    let center, radius;
    cv.minEnclosingCircle(rect, c => (center = c), r => (radius = r));

    const expectedRadius = Math.hypot(RECT.width, RECT.height) / 2; // half the diagonal
    assert(Math.abs(center.x - 60) < 5, `expected center.x ~60, got ${center.x}`);
    assert(Math.abs(center.y - 60) < 5, `expected center.y ~60, got ${center.y}`);
    assert(Math.abs(radius - expectedRadius) < expectedRadius * 0.1, `expected radius ~${expectedRadius}, got ${radius}`);
  });

  addTest(`minEnclosingTriangle - rectangle (${label})`, () => {
    const triangle = new cv.Mat();
    const area = cv.minEnclosingTriangle(rect, triangle);
    assert(area >= RECT_AREA, `enclosing triangle area (${area}) should be >= rectangle area (${RECT_AREA})`);
    eq(3, triangle.total());
  });

  addTest(`fitLine - rectangle (${label})`, () => {
    const line = new cv.Mat();
    cv.fitLine(rect, line, 2 /* cv.DIST_L2 */, 0, 0.01, 0.01);
    eq(4, line.total()); // (vx, vy, x0, y0)
  });

  addTest(`matchShapes - rectangle vs itself (${label})`, () => {
    const match = cv.matchShapes(rect, rect, cv.CONTOURS_MATCH_I1, 0);
    assert(match < 1e-6, `expected ~0 for identical shapes, got ${match}`);
  });

  addTest(`intersectConvexConvex - rectangle vs itself (${label})`, () => {
    const intersection = new cv.Mat();
    const area = cv.intersectConvexConvex(rect, rect, intersection, true);
    assert(Math.abs(area - RECT_AREA) < RECT_AREA * 0.1, `expected intersection area ~${RECT_AREA}, got ${area}`);
  });
}

for (const [label, circle] of circleByRepresentation) {
  const diameter = 2 * CIRCLE_RADIUS;

  addTest(`fitEllipse - circle (${label})`, () => {
    const ellipse = cv.fitEllipse(circle);
    assert(Math.abs(ellipse.center.x - CIRCLE_CENTER.x) < 5, `expected center.x ~${CIRCLE_CENTER.x}, got ${ellipse.center.x}`);
    assert(Math.abs(ellipse.center.y - CIRCLE_CENTER.y) < 5, `expected center.y ~${CIRCLE_CENTER.y}, got ${ellipse.center.y}`);
    assert(Math.abs(ellipse.size.width - diameter) < diameter * 0.15, `expected width ~${diameter}, got ${ellipse.size.width}`);
    assert(Math.abs(ellipse.size.height - diameter) < diameter * 0.15, `expected height ~${diameter}, got ${ellipse.size.height}`);
  });

  addTest(`fitEllipseAMS - circle (${label})`, () => {
    const ellipse = cv.fitEllipseAMS(circle);
    assert(Math.abs(ellipse.size.width - diameter) < diameter * 0.15, `expected width ~${diameter}, got ${ellipse.size.width}`);
    assert(Math.abs(ellipse.size.height - diameter) < diameter * 0.15, `expected height ~${diameter}, got ${ellipse.size.height}`);
  });

  addTest(`fitEllipseDirect - circle (${label})`, () => {
    const ellipse = cv.fitEllipseDirect(circle);
    assert(Math.abs(ellipse.size.width - diameter) < diameter * 0.15, `expected width ~${diameter}, got ${ellipse.size.width}`);
    assert(Math.abs(ellipse.size.height - diameter) < diameter * 0.15, `expected height ~${diameter}, got ${ellipse.size.height}`);
  });
}

tests(testCases);

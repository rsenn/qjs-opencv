import { tests, assert } from './tinytest.js';
import * as cv from 'opencv';

/*
 * Exercises the geometric-transform and shape-detection bindings
 * referenced by the opencv.js example pages cataloged in
 * doc/opencv-js-examples.md (js_imgproc: geometric transforms, and the
 * Hough-transform half of shape detection). Synthetic Mats only.
 *
 * cv.matchTemplate and the cv.TM_* constants have their own dedicated
 * test_template_matching.js.
 */

const testCases = {};
function addTest(name, fn) {
  if (testCases[name]) throw new Error(`duplicate test name: ${name}`);
  testCases[name] = fn;
}

function shapeImage(size) {
  const img = cv.Mat.zeros(size, size, cv.CV_8UC1);
  cv.rectangle(img, { x: size / 4, y: size / 4, width: size / 2, height: size / 2 }, 255, -1);
  return img;
}

addTest('resize - INTER_AREA downscale', () => {
  const src = shapeImage(40);
  const dst = new cv.Mat();
  cv.resize(src, dst, new cv.Size(20, 20), 0, 0, cv.INTER_AREA);
  assert(dst.rows === 20 && dst.cols === 20, `expected 20x20, got ${dst.rows}x${dst.cols}`);
});

addTest('warpAffine - translation via matFromArray', () => {
  const src = shapeImage(40);
  const dst = new cv.Mat();
  const M = cv.matFromArray(2, 3, cv.CV_64FC1, [1, 0, 5, 0, 1, 5]);
  cv.warpAffine(src, dst, M, new cv.Size(40, 40), cv.INTER_LINEAR, cv.BORDER_CONSTANT, [0, 0, 0, 0]);
  assert(dst.rows === 40 && dst.cols === 40, 'expected same output size');
});

addTest('getRotationMatrix2D + warpAffine - rotate about center', () => {
  const src = shapeImage(40);
  const M = cv.getRotationMatrix2D(new cv.Point(20, 20), 45, 1.0);
  assert(M.rows === 2 && M.cols === 3, `expected a 2x3 rotation matrix, got ${M.rows}x${M.cols}`);
  const dst = new cv.Mat();
  cv.warpAffine(src, dst, M, new cv.Size(40, 40), cv.INTER_LINEAR, cv.BORDER_CONSTANT, [0, 0, 0, 0]);
  assert(dst.rows === 40 && dst.cols === 40, 'expected same output size');
});

addTest('getAffineTransform + warpAffine - 3-point correspondence', () => {
  const src = shapeImage(40);
  const srcPts = [new cv.Point(0, 0), new cv.Point(39, 0), new cv.Point(0, 39)];
  const dstPts = [new cv.Point(0, 0), new cv.Point(39, 0), new cv.Point(5, 39)];
  const M = cv.getAffineTransform(srcPts, dstPts);
  assert(M.rows === 2 && M.cols === 3, `expected a 2x3 affine matrix, got ${M.rows}x${M.cols}`);
  const dst = new cv.Mat();
  cv.warpAffine(src, dst, M, new cv.Size(40, 40), cv.INTER_LINEAR, cv.BORDER_CONSTANT, [0, 0, 0, 0]);
  assert(dst.rows === 40 && dst.cols === 40, 'expected same output size');
});

addTest('getPerspectiveTransform + warpPerspective - homography warp', () => {
  const src = shapeImage(40);
  const srcPts = [new cv.Point(0, 0), new cv.Point(39, 0), new cv.Point(39, 39), new cv.Point(0, 39)];
  const dstPts = [new cv.Point(2, 2), new cv.Point(37, 0), new cv.Point(39, 39), new cv.Point(0, 37)];
  const M = cv.getPerspectiveTransform(srcPts, dstPts);
  assert(M.rows === 3 && M.cols === 3, `expected a 3x3 perspective matrix, got ${M.rows}x${M.cols}`);
  const dst = new cv.Mat();
  cv.warpPerspective(src, dst, M, new cv.Size(40, 40), cv.INTER_LINEAR, cv.BORDER_CONSTANT, [0, 0, 0, 0]);
  assert(dst.rows === 40 && dst.cols === 40, 'expected same output size');
});

addTest('HoughLines - standard Hough transform', () => {
  const img = cv.Mat.zeros(60, 60, cv.CV_8UC1);
  cv.line(img, new cv.Point(0, 30), new cv.Point(59, 30), 255, 1);
  const lines = [];
  cv.HoughLines(img, lines, 1, Math.PI / 180, 40);
  assert(lines.length > 0, 'expected at least one detected line');
  assert(lines[0].length === 2, 'expected each line as a [rho, theta] pair');
});

addTest('HoughLinesP - probabilistic Hough transform', () => {
  const img = cv.Mat.zeros(60, 60, cv.CV_8UC1);
  cv.line(img, new cv.Point(0, 30), new cv.Point(59, 30), 255, 1);
  const lines = new cv.Mat();
  cv.HoughLinesP(img, lines, 1, Math.PI / 180, 40, 30, 5);
  assert(lines.rows > 0, `expected at least one detected line segment, got ${lines.rows}`);
});

addTest('HoughCircles - circle detection', () => {
  const img = cv.Mat.zeros(80, 80, cv.CV_8UC1);
  cv.circle(img, new cv.Point(40, 40), 20, 255, 2);
  const circles = new cv.Mat();
  cv.HoughCircles(img, circles, cv.HOUGH_GRADIENT, 1, 40, 100, 20, 10, 30);
  assert(circles.cols > 0 || circles.rows > 0, 'expected at least one detected circle');
});

tests(testCases);

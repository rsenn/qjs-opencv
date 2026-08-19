import { tests, eq, assert } from './tinytest.js';
import * as cv from 'opencv';

/*
 * Exercises the core/arithmetic and color/threshold/histogram bindings
 * referenced by the opencv.js example pages cataloged in
 * doc/opencv-js-examples.md (js_setup, js_gui, js_core, and the
 * color/thresholding half of js_imgproc). Synthetic Mats only - no file
 * I/O - so these run anywhere.
 *
 * Not testable here (genuinely unbound - see TODO.md's "opencv.js Example
 * Compatibility Gaps" section and BUGS' opencvjs-* entries):
 * cv.calcBackProject, cv.TermCriteria.
 */

const testCases = {};
function addTest(name, fn) {
  if (testCases[name]) throw new Error(`duplicate test name: ${name}`);
  testCases[name] = fn;
}

function rgbaImage(size) {
  const img = new cv.Mat(size, size, cv.CV_8UC4);
  img.setTo([0, 0, 0, 255]);
  const margin = Math.floor(size / 4);
  cv.rectangle(img, { x: margin, y: margin, width: size - 2 * margin, height: size - 2 * margin }, [200, 50, 50, 255], -1);
  return img;
}

addTest('Mat - ROI copy (basic_ops_roi)', () => {
  const src = cv.Mat.zeros(100, 100, cv.CV_8UC1);
  cv.rectangle(src, { x: 20, y: 20, width: 30, height: 30 }, 255, -1);
  const roi = src.roi(new cv.Rect(20, 20, 30, 30));
  assert(roi.rows === 30 && roi.cols === 30, `expected 30x30 ROI, got ${roi.rows}x${roi.cols}`);
  assert(roi.data[0] === 255, 'expected ROI to view the filled region');
});

addTest('copyMakeBorder - constant border', () => {
  const src = cv.Mat.ones(10, 10, cv.CV_8UC1);
  const dst = new cv.Mat();
  cv.copyMakeBorder(src, dst, 5, 5, 5, 5, cv.BORDER_CONSTANT, [7, 0, 0, 0]);
  assert(dst.rows === 20 && dst.cols === 20, `expected 20x20, got ${dst.rows}x${dst.cols}`);
  assert(dst.data[0] === 7, `expected border value 7, got ${dst.data[0]}`);
});

addTest('bitwise_and / bitwise_not / add (arithmetics_bitwise)', () => {
  const a = cv.Mat.zeros(10, 10, cv.CV_8UC1);
  cv.rectangle(a, { x: 0, y: 0, width: 5, height: 10 }, 255, -1);
  const b = cv.Mat.zeros(10, 10, cv.CV_8UC1);
  cv.rectangle(b, { x: 3, y: 0, width: 5, height: 10 }, 255, -1);

  const anded = new cv.Mat();
  cv.bitwise_and(a, b, anded);
  assert(anded.data[3] === 255, 'expected overlap column to be 255');
  assert(anded.data[0] === 0, 'expected non-overlap column to be 0');

  const notA = new cv.Mat();
  cv.bitwise_not(a, notA);
  assert(notA.data[0] === 0 && notA.data[9] === 255, 'expected bitwise_not to invert');

  const summed = new cv.Mat();
  cv.add(a, b, summed);
  assert(summed.data[3] === 255, 'expected add() to saturate at 255 in the overlap');
});

addTest('cvtColor - COLOR_RGBA2GRAY', () => {
  const img = rgbaImage(20);
  const gray = new cv.Mat();
  cv.cvtColor(img, gray, cv.COLOR_RGBA2GRAY);
  assert(gray.channels() === 1, `expected 1 channel, got ${gray.channels()}`);
  assert(gray.rows === 20 && gray.cols === 20, 'expected same spatial size');
});

addTest('inRange - color mask', () => {
  const img = rgbaImage(20);
  const mask = new cv.Mat();
  // Bounds passed as plain JS Arrays currently hit a dangling-pointer bug
  // in js_cv_inputarray()'s Scalar path (silently produces an all-zero
  // mask - see BUGS: js-cv-inputarray-scalar-dangling-pointer). Float64Array
  // bounds take a different, safe code path, so use that here.
  cv.inRange(img, Float64Array.of(150, 0, 0, 0), Float64Array.of(255, 100, 100, 255), mask);
  assert(mask.data[10 * 20 + 10] === 255, 'expected center pixel to be in range');
  assert(mask.data[0] === 0, 'expected corner pixel to be out of range');
});

addTest('threshold - THRESH_BINARY', () => {
  const src = cv.matFromArray(1, 4, cv.CV_8UC1, [10, 200, 50, 250]);
  const dst = new cv.Mat();
  cv.threshold(src, dst, 128, 255, cv.THRESH_BINARY);
  assert(JSON.stringify([...dst.data]) === JSON.stringify([0, 255, 0, 255]), `got ${[...dst.data]}`);
});

addTest('adaptiveThreshold - ADAPTIVE_THRESH_GAUSSIAN_C', () => {
  const img = rgbaImage(40);
  const gray = new cv.Mat();
  cv.cvtColor(img, gray, cv.COLOR_RGBA2GRAY);
  const dst = new cv.Mat();
  cv.adaptiveThreshold(gray, dst, 255, cv.ADAPTIVE_THRESH_GAUSSIAN_C, cv.THRESH_BINARY, 11, 2);
  assert(dst.rows === 40 && dst.cols === 40, 'expected same spatial size');
  assert(dst.type() === cv.CV_8UC1, 'expected single-channel 8U output');
});

addTest('calcHist + minMaxLoc - grayscale histogram', () => {
  const gray = cv.Mat.zeros(10, 10, cv.CV_8UC1);
  cv.rectangle(gray, { x: 0, y: 0, width: 10, height: 5 }, 200, -1);
  const mask = new cv.Mat();
  const hist = new cv.Mat();
  // `uniform` (the 8th arg) must be passed explicitly despite its stated
  // default - see BUGS: calchist-requires-8-args-throws-null.
  cv.calcHist([gray], [0], mask, hist, 1, [256], [[0, 256]], true);
  assert(hist.rows * hist.cols === 256, `expected a 256-bin histogram, got ${hist.rows}x${hist.cols}`);
  const { maxLoc } = cv.minMaxLoc(hist);
  const peakBin = Math.max(maxLoc.x, maxLoc.y);
  assert(peakBin === 0 || peakBin === 200, `expected the histogram peak at bin 0 or 200, got bin ${peakBin}`);
});

addTest('equalizeHist - contrast stretch', () => {
  const gray = cv.Mat.zeros(20, 20, cv.CV_8UC1);
  cv.rectangle(gray, { x: 0, y: 0, width: 20, height: 10 }, 100, -1);
  cv.rectangle(gray, { x: 0, y: 10, width: 20, height: 10 }, 150, -1);
  const dst = new cv.Mat();
  cv.equalizeHist(gray, dst);
  assert(dst.rows === 20 && dst.cols === 20, 'expected same spatial size');
});

addTest('createCLAHE - local contrast', () => {
  const gray = cv.Mat.zeros(20, 20, cv.CV_8UC1);
  cv.rectangle(gray, { x: 0, y: 0, width: 20, height: 10 }, 100, -1);
  const clahe = cv.createCLAHE(2.0, new cv.Size(4, 4));
  const dst = new cv.Mat();
  clahe.apply(gray, dst);
  assert(dst.rows === 20 && dst.cols === 20, 'expected same spatial size');
});

tests(testCases);

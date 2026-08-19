import { tests, assert } from './tinytest.js';
import * as cv from 'opencv';

/*
 * Exercises the filtering/morphology/edge-detection bindings referenced by
 * the opencv.js example pages cataloged in doc/opencv-js-examples.md
 * (js_imgproc: filtering & morphology, plus pyramids). Synthetic Mats
 * only - no file I/O.
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

addTest('blur / boxFilter - box smoothing', () => {
  const src = shapeImage(40);
  const dstBlur = new cv.Mat();
  cv.blur(src, dstBlur, new cv.Size(5, 5));
  assert(dstBlur.rows === 40 && dstBlur.cols === 40, 'expected same spatial size');

  const dstBox = new cv.Mat();
  cv.boxFilter(src, dstBox, -1, new cv.Size(5, 5));
  assert(dstBox.rows === 40 && dstBox.cols === 40, 'expected same spatial size');
});

addTest('GaussianBlur - gaussian smoothing', () => {
  const src = shapeImage(40);
  const dst = new cv.Mat();
  cv.GaussianBlur(src, dst, new cv.Size(5, 5), 0);
  assert(dst.rows === 40 && dst.cols === 40, 'expected same spatial size');
});

addTest('medianBlur - salt-and-pepper removal', () => {
  const src = shapeImage(40);
  const dst = new cv.Mat();
  cv.medianBlur(src, dst, 5);
  assert(dst.rows === 40 && dst.cols === 40, 'expected same spatial size');
});

addTest('bilateralFilter - edge-preserving smoothing', () => {
  const src = shapeImage(40);
  const color = new cv.Mat();
  cv.cvtColor(src, color, cv.COLOR_GRAY2BGR);
  const dst = new cv.Mat();
  cv.bilateralFilter(color, dst, 9, 75, 75);
  assert(dst.rows === 40 && dst.cols === 40, 'expected same spatial size');
});

addTest('filter2D - arbitrary kernel convolution', () => {
  const src = shapeImage(40);
  const kernel = cv.matFromArray(3, 3, cv.CV_32FC1, [0, 0, 0, 0, 1, 0, 0, 0, 0]);
  const dst = new cv.Mat();
  cv.filter2D(src, dst, -1, kernel);
  assert(dst.rows === 40 && dst.cols === 40, 'expected same spatial size');
  assert(dst.data[20 * 40 + 20] === src.data[20 * 40 + 20], 'identity kernel should reproduce the input');
});

addTest('erode - shrink bright regions', () => {
  const src = shapeImage(40);
  const kernel = cv.getStructuringElement(cv.MORPH_RECT, new cv.Size(3, 3));
  const dst = new cv.Mat();
  cv.erode(src, dst, kernel);
  const before = cv.countNonZero(src);
  const after = cv.countNonZero(dst);
  assert(after < before, `expected erosion to shrink the bright area (${after} >= ${before})`);
});

addTest('dilate - grow bright regions', () => {
  const src = shapeImage(40);
  const kernel = cv.getStructuringElement(cv.MORPH_RECT, new cv.Size(3, 3));
  const dst = new cv.Mat();
  cv.dilate(src, dst, kernel);
  const before = cv.countNonZero(src);
  const after = cv.countNonZero(dst);
  assert(after > before, `expected dilation to grow the bright area (${after} <= ${before})`);
});

addTest('getStructuringElement - custom kernel shapes', () => {
  const rect = cv.getStructuringElement(cv.MORPH_RECT, new cv.Size(5, 5));
  const cross = cv.getStructuringElement(cv.MORPH_CROSS, new cv.Size(5, 5));
  assert(cv.countNonZero(rect) === 25, `expected a full 5x5 rect kernel, got ${cv.countNonZero(rect)} set pixels`);
  assert(cv.countNonZero(cross) < 25, `expected a sparser cross kernel, got ${cv.countNonZero(cross)} set pixels`);
});

for (const [opName, op] of [
  ['morphologyEx - MORPH_OPEN', cv.MORPH_OPEN],
  ['morphologyEx - MORPH_CLOSE', cv.MORPH_CLOSE],
  ['morphologyEx - MORPH_GRADIENT', cv.MORPH_GRADIENT],
  ['morphologyEx - MORPH_TOPHAT', cv.MORPH_TOPHAT],
  ['morphologyEx - MORPH_BLACKHAT', cv.MORPH_BLACKHAT],
]) {
  addTest(opName, () => {
    const src = shapeImage(40);
    const kernel = cv.getStructuringElement(cv.MORPH_RECT, new cv.Size(3, 3));
    const dst = new cv.Mat();
    cv.morphologyEx(src, dst, op, kernel);
    assert(dst.rows === 40 && dst.cols === 40, 'expected same spatial size');
  });
}

addTest('Sobel - gradient edge detection', () => {
  const src = shapeImage(40);
  const dst = new cv.Mat();
  cv.Sobel(src, dst, cv.CV_64F, 1, 0);
  assert(dst.rows === 40 && dst.cols === 40, 'expected same spatial size');
  assert(cv.countNonZero(dst) > 0, 'expected nonzero gradient response at the rectangle edge');
});

addTest('convertScaleAbs - Sobel response back to displayable 8U', () => {
  const src = shapeImage(40);
  const sobel = new cv.Mat();
  cv.Sobel(src, sobel, cv.CV_64F, 1, 0);
  const dst = new cv.Mat();
  cv.convertScaleAbs(sobel, dst);
  assert(dst.type() === cv.CV_8UC1, 'expected an 8U result');
});

addTest('Canny - edge detection', () => {
  const src = shapeImage(40);
  const dst = new cv.Mat();
  cv.Canny(src, dst, 50, 150);
  assert(dst.rows === 40 && dst.cols === 40, 'expected same spatial size');
  assert(cv.countNonZero(dst) > 0, 'expected edges to be detected around the rectangle');
});

addTest('Laplacian - second-derivative edge detection', () => {
  const src = shapeImage(40);
  const dst = new cv.Mat();
  cv.Laplacian(src, dst, cv.CV_64F);
  assert(dst.rows === 40 && dst.cols === 40, 'expected same spatial size');
  assert(cv.countNonZero(dst) > 0, 'expected a nonzero response at the rectangle edge');
});

addTest('Scharr - gradient edge detection', () => {
  const src = shapeImage(40);
  const dst = new cv.Mat();
  cv.Scharr(src, dst, cv.CV_64F, 1, 0);
  assert(dst.rows === 40 && dst.cols === 40, 'expected same spatial size');
  assert(cv.countNonZero(dst) > 0, 'expected a nonzero horizontal-gradient response');
});

addTest('pyrDown - downsample (blur + halve)', () => {
  const src = shapeImage(40);
  const dst = new cv.Mat();
  cv.pyrDown(src, dst);
  assert(dst.rows === 20 && dst.cols === 20, `expected 20x20, got ${dst.rows}x${dst.cols}`);
});

addTest('pyrUp - upsample (double + blur)', () => {
  const src = shapeImage(40);
  const dst = new cv.Mat();
  cv.pyrUp(src, dst);
  assert(dst.rows === 80 && dst.cols === 80, `expected 80x80, got ${dst.rows}x${dst.cols}`);
});

tests(testCases);

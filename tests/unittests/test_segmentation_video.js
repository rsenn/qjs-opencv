import { tests, assert } from './tinytest.js';
import * as cv from 'opencv';

/*
 * Exercises the segmentation/misc and video-analysis bindings referenced
 * by the opencv.js example pages cataloged in doc/opencv-js-examples.md
 * (js_imgproc: segmentation & misc, and js_video). Synthetic Mats only.
 *
 * Not testable here (genuinely unbound - see TODO.md's "opencv.js Example
 * Compatibility Gaps" section and BUGS' opencvjs-* entries):
 * cv.calcBackProject, cv.calcOpticalFlowPyrLK, cv.calcOpticalFlowFarneback,
 * cv.getOptimalDFTSize, cv.segmentation_IntelligentScissorsMB.
 *
 * cv.TermCriteria's own constructor contract has its own dedicated
 * test_termcriteria.js; the meanShift/CamShift tests below just use it as
 * an ordinary argument, same as any real opencv.js tracking snippet would.
 *
 * cv.BackgroundSubtractorMOG2 isn't directly constructible (see TODO.md) -
 * uses the working cv.createBackgroundSubtractorMOG2() factory instead.
 * cv.rotatedRectPoints() doesn't exist as a free function (see TODO.md) -
 * uses the working RotatedRect.points() instance method instead.
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

addTest('distanceTransform - distance to nearest zero pixel', () => {
  const src = shapeImage(40);
  const dst = new cv.Mat();
  const labels = new cv.Mat();
  // cv.DIST_L2 isn't bound as a constant (see TODO.md) - pass the literal
  // enum value (cv::DIST_L2 == 2) instead.
  cv.distanceTransform(src, dst, labels, 2 /* DIST_L2 */, 5);
  assert(dst.rows === 40 && dst.cols === 40, 'expected same spatial size');
  assert(dst.data32F[20 * 40 + 20] > 0, 'expected a positive distance inside the filled rectangle');
});

addTest('connectedComponents - label distinct blobs', () => {
  const src = cv.Mat.zeros(40, 40, cv.CV_8UC1);
  cv.rectangle(src, { x: 2, y: 2, width: 8, height: 8 }, 255, -1);
  cv.rectangle(src, { x: 30, y: 30, width: 8, height: 8 }, 255, -1);
  const labels = new cv.Mat();
  const n = cv.connectedComponents(src, labels, 8, cv.CV_32S, -1 /* cv::CCL_DEFAULT */);
  assert(n === 3, `expected background + 2 blobs = 3 labels, got ${n}`);
});

addTest('watershed - marker-based segmentation', () => {
  const color = cv.Mat.zeros(40, 40, cv.CV_8UC3);
  cv.rectangle(color, { x: 2, y: 2, width: 15, height: 36 }, [255, 0, 0], -1);
  cv.rectangle(color, { x: 22, y: 2, width: 15, height: 36 }, [0, 255, 0], -1);

  const markers = cv.Mat.zeros(40, 40, cv.CV_32S);
  cv.rectangle(markers, { x: 5, y: 5, width: 5, height: 5 }, 1, -1);
  cv.rectangle(markers, { x: 28, y: 28, width: 5, height: 5 }, 2, -1);
  cv.rectangle(markers, { x: 0, y: 0, width: 40, height: 1 }, 3, -1);

  cv.watershed(color, markers);
  assert(markers.data32S[7 * 40 + 7] === 1, 'expected the first marker region to keep its label');
});

addTest('grabCut - rectangle-initialized foreground extraction', () => {
  const color = cv.Mat.zeros(30, 30, cv.CV_8UC3);
  cv.rectangle(color, { x: 5, y: 5, width: 20, height: 20 }, [200, 200, 200], -1);
  const mask = new cv.Mat();
  const bgdModel = new cv.Mat();
  const fgdModel = new cv.Mat();
  cv.grabCut(color, mask, { x: 5, y: 5, width: 20, height: 20 }, bgdModel, fgdModel, 1, cv.GC_INIT_WITH_RECT);
  assert(mask.rows === 30 && mask.cols === 30, 'expected a full-size mask to be produced');
});

addTest('dft + magnitude - frequency-domain analysis', () => {
  const src = shapeImage(32);
  const srcFloat = new cv.Mat();
  src.convertTo(srcFloat, cv.CV_32F);
  const planes = [srcFloat, cv.Mat.zeros(32, 32, cv.CV_32F)];
  const complex = new cv.Mat();
  cv.merge(planes, complex);
  cv.dft(complex, complex);
  const split = [];
  cv.split(complex, split);
  const mag = new cv.Mat();
  cv.magnitude(split[0], split[1], mag);
  assert(mag.rows === 32 && mag.cols === 32, 'expected same spatial size');
});

addTest('normalize - NORM_MINMAX stretch', () => {
  const src = cv.matFromArray(1, 4, cv.CV_32FC1, [10, 20, 30, 40]);
  const dst = new cv.Mat();
  cv.normalize(src, dst, 0, 1, cv.NORM_MINMAX);
  assert(Math.abs(dst.data32F[0]) < 1e-6, `expected min to normalize to 0, got ${dst.data32F[0]}`);
  assert(Math.abs(dst.data32F[3] - 1) < 1e-6, `expected max to normalize to 1, got ${dst.data32F[3]}`);
});

addTest('polylines - open polyline', () => {
  const img = cv.Mat.zeros(40, 40, cv.CV_8UC1);
  const pts = cv.matFromArray(4, 1, cv.CV_32SC2, [5, 5, 35, 5, 35, 35, 5, 35]);
  // js_draw_polylines's color argument silently draws nothing unless
  // given a full 4-element array, even on a single-channel image - see
  // BUGS: polylines-color-requires-4-element-array.
  cv.polylines(img, pts, true, [255, 255, 255, 255], 1);
  assert(cv.countNonZero(img) > 0, 'expected the polyline to draw nonzero pixels');
});

addTest('createBackgroundSubtractorMOG2 + apply - foreground mask', () => {
  const mog2 = cv.createBackgroundSubtractorMOG2(500, 16, true);
  const frame1 = cv.Mat.zeros(20, 20, cv.CV_8UC3);
  const fgmask = new cv.Mat();
  mog2.apply(frame1, fgmask);
  assert(fgmask.rows === 20 && fgmask.cols === 20, 'expected a full-size foreground mask');
});

addTest('meanShift - fixed-window object tracking', () => {
  const prob = cv.Mat.zeros(60, 60, cv.CV_8UC1);
  cv.rectangle(prob, { x: 25, y: 25, width: 20, height: 20 }, 200, -1);
  const window = new cv.Rect(20, 20, 24, 24);
  const criteria = new cv.TermCriteria(cv.TERM_CRITERIA_EPS + cv.TERM_CRITERIA_COUNT, 10, 1);
  const [n, updated] = cv.meanShift(prob, window, criteria);
  assert(typeof n === 'number', 'expected meanShift to return an iteration count');
  assert(updated.x >= 20 && updated.x <= 30, `expected the window to stay near the bright blob, got x=${updated.x}`);
});

addTest('CamShift - adaptive-window object tracking', () => {
  const prob = cv.Mat.zeros(60, 60, cv.CV_8UC1);
  cv.rectangle(prob, { x: 25, y: 25, width: 20, height: 20 }, 200, -1);
  const window = new cv.Rect(20, 20, 24, 24);
  const criteria = new cv.TermCriteria(cv.TERM_CRITERIA_EPS + cv.TERM_CRITERIA_COUNT, 10, 1);
  const [rr, updated] = cv.CamShift(prob, window, criteria);
  const pts = rr.points();
  assert(pts.length === 4, 'expected a RotatedRect with 4 corner points');
  assert(updated.x >= 15 && updated.x <= 35, `expected the window to stay near the bright blob, got x=${updated.x}`);
});

addTest('goodFeaturesToTrack - corner detection', () => {
  const img = shapeImage(60);
  const corners = new cv.Mat();
  cv.goodFeaturesToTrack(img, corners, 20, 0.01, 10);
  assert(corners.rows > 0, `expected at least one corner to be found, got ${corners.rows}`);
});

addTest('cartToPolar - vector field to magnitude/angle', () => {
  const x = cv.matFromArray(1, 2, cv.CV_32FC1, [3, 0]);
  const y = cv.matFromArray(1, 2, cv.CV_32FC1, [4, 5]);
  const mag = new cv.Mat();
  const angle = new cv.Mat();
  cv.cartToPolar(x, y, mag, angle);
  assert(Math.abs(mag.data32F[0] - 5) < 1e-4, `expected magnitude(3,4) == 5, got ${mag.data32F[0]}`);
});

tests(testCases);

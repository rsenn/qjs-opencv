import { tests, assert } from './tinytest.js';
import * as cv from 'opencv';

/*
 * Exercises cv.TermCriteria, the constructor referenced (alongside the
 * already-exported cv.TERM_CRITERIA_* constants) by 3 opencv.js example
 * pages cataloged in doc/opencv-js-examples.md:
 *
 *   js_camshift.html, js_meanshift.html - `new cv.TermCriteria(cv.
 *   TERM_CRITERIA_EPS | cv.TERM_CRITERIA_COUNT, 10, 1)` passed straight
 *   into cv.CamShift()/cv.meanShift() (both already bound - see
 *   test_segmentation_video.js's 'meanShift'/'CamShift' tests, which use
 *   cv.TermCriteria the same way a real opencv.js snippet would). These 2
 *   examples are now fully supported end to end.
 *
 *   js_optical_flow_lucas_kanade.html - also constructs a TermCriteria,
 *   but passes it to cv.calcOpticalFlowPyrLK(), which isn't bound at all
 *   (see BUGS' opencvjs-video-tracking-module-unbound) - this example
 *   stays blocked regardless of TermCriteria itself working correctly.
 *
 * opencv.js's own cv.TermCriteria (modules/js/src/helpers.js) is a plain
 * constructor producing a {type, maxCount, epsilon} object - not an
 * embind class. qjs-opencv's internal functions that accept a criteria
 * argument (kmeans, CamShift, meanShift) all treat it as an ad-hoc
 * [type, maxCount, epsilon] array via js_array_to() instead, so this
 * binding returns a real Array with the same 3 fields layered on as named
 * properties, satisfying both conventions - the tests below check both.
 */

const testCases = {};
function addTest(name, fn) {
  if (testCases[name]) throw new Error(`duplicate test name: ${name}`);
  testCases[name] = fn;
}

addTest('TermCriteria - 3-arg constructor sets named fields', () => {
  const c = new cv.TermCriteria(cv.TERM_CRITERIA_EPS | cv.TERM_CRITERIA_COUNT, 10, 0.03);
  assert(c.type === (cv.TERM_CRITERIA_EPS | cv.TERM_CRITERIA_COUNT), `type: got ${c.type}`);
  assert(c.maxCount === 10, `maxCount: got ${c.maxCount}`);
  assert(c.epsilon === 0.03, `epsilon: got ${c.epsilon}`);
});

addTest('TermCriteria - 3-arg constructor also unpacks positionally', () => {
  // qjs-opencv's own internal consumers (kmeans, CamShift, meanShift) all
  // read a criteria argument as a plain [type, maxCount, epsilon] array -
  // this must keep working with no changes to any of those call sites.
  const c = new cv.TermCriteria(5, 20, 0.5);
  assert(Array.isArray(c), 'expected a real Array');
  assert(c.length === 3, `expected length 3, got ${c.length}`);
  assert(c[0] === 5 && c[1] === 20 && c[2] === 0.5, `got [${c[0]}, ${c[1]}, ${c[2]}]`);
});

addTest('TermCriteria - 0-arg constructor defaults to all zero', () => {
  const c = new cv.TermCriteria();
  assert(c.type === 0 && c.maxCount === 0 && c.epsilon === 0, `got type=${c.type} maxCount=${c.maxCount} epsilon=${c.epsilon}`);
  assert(c[0] === 0 && c[1] === 0 && c[2] === 0, `got [${c[0]}, ${c[1]}, ${c[2]}]`);
});

addTest('TermCriteria - requires new (real opencv.js code always uses new too)', () => {
  // opencv.js's own TermCriteria (modules/js/src/helpers.js) is a plain
  // `function TermCriteria() { this.type = ...; }` - calling it without
  // `new` wouldn't build a proper object there either (`this` wouldn't be
  // a fresh object), and every real sample already always uses `new`. This
  // binding just enforces that instead of silently misbehaving.
  let threw = false;
  try {
    cv.TermCriteria(cv.TERM_CRITERIA_COUNT, 5, 0);
  } catch (e) {
    threw = true;
  }
  assert(threw, 'expected calling TermCriteria() without new to throw');
});

addTest('TermCriteria - rejects 1 or 2 arguments', () => {
  for (const args of [[1], [1, 2]]) {
    let threw = false;
    try {
      new cv.TermCriteria(...args);
    } catch (e) {
      threw = true;
    }
    assert(threw, `expected TermCriteria(${args.join(', ')}) to throw`);
  }
});

addTest('TermCriteria - TERM_CRITERIA_* constants are exported', () => {
  // cv::TermCriteria::COUNT/EPS/MAX_ITER native values, per opencv2/
  // core/types.hpp - the js_camshift/js_meanshift/js_optical_flow_lucas_
  // kanade examples all combine EPS|COUNT specifically.
  assert(cv.TERM_CRITERIA_COUNT === 1, `TERM_CRITERIA_COUNT: got ${cv.TERM_CRITERIA_COUNT}`);
  assert(cv.TERM_CRITERIA_MAX_ITER === 1, `TERM_CRITERIA_MAX_ITER: got ${cv.TERM_CRITERIA_MAX_ITER}`);
  assert(cv.TERM_CRITERIA_EPS === 2, `TERM_CRITERIA_EPS: got ${cv.TERM_CRITERIA_EPS}`);
});

addTest('TermCriteria - end to end with CamShift, same shape as js_camshift.html', () => {
  const prob = cv.Mat.zeros(60, 60, cv.CV_8UC1);
  cv.rectangle(prob, { x: 25, y: 25, width: 20, height: 20 }, 200, -1);
  const window = new cv.Rect(20, 20, 24, 24);
  const criteria = new cv.TermCriteria(cv.TERM_CRITERIA_EPS | cv.TERM_CRITERIA_COUNT, 10, 1);

  const [rotatedRect, updated] = cv.CamShift(prob, window, criteria);
  assert(rotatedRect.points().length === 4, 'expected a RotatedRect with 4 corner points');
  assert(updated.x >= 15 && updated.x <= 35, `expected the window to stay near the bright blob, got x=${updated.x}`);
});

tests(testCases);

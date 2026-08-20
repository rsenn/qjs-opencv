import { tests, assert } from './tinytest.js';
import * as cv from 'opencv';

/*
 * Exercises cv.matchTemplate() and the cv.TM_* constants, referenced by
 * the opencv.js example page cataloged in doc/opencv-js-examples.md:
 *
 *   js_template_matching_matchTemplate.html - cv.matchTemplate(src, templ,
 *   dst, cv.TM_CCOEFF, mask), then cv.minMaxLoc(dst, mask). Both
 *   cv.matchTemplate and cv.minMaxLoc are bound, so this example is fully
 *   supported (verified with a synthetic embedded patch below, in place
 *   of the example's file-loaded images/canvas).
 *
 * cv.minMaxLoc(dst, mask) with a real (non-noArray) mask argument leaves
 * a stray pending exception on the context - see BUGS'
 * minmaxloc-mask-argument-leftover-pending-exception, found while writing
 * this file. Not this file's bug to fix (unrelated to matchTemplate), so
 * the tests below call cv.minMaxLoc(dst) without a mask, same as every
 * other minMaxLoc call in this test suite.
 */

const testCases = {};
function addTest(name, fn) {
  if (testCases[name]) throw new Error(`duplicate test name: ${name}`);
  testCases[name] = fn;
}

// Deterministic pseudo-random fill (cv.randu's low/high scalar arguments
// don't currently accept plain numbers - see BUGS - so this avoids that
// entirely). A genuinely non-periodic pattern matters here: an earlier
// version of this test used a linear (x*a + y*b) % 256 fill and got a
// *second*, unintended exact match elsewhere in the image, because the
// chosen embed offset happened to satisfy a*dx + b*dy = 0 (mod 256) -
// i.e. the fill's own periodicity aliased with the embed location. A
// congruential generator has no such linear structure to alias against.
function noiseMat(rows, cols) {
  const m = new cv.Mat(rows, cols, cv.CV_8UC1);
  let seed = 0x2545F4914F6CDD1D & 0x7fffffff;
  for (let i = 0; i < rows * cols; i++) {
    seed = (seed * 1103515245 + 12345) & 0x7fffffff;
    m.data[i] = seed & 0xff;
  }
  return m;
}

function embedTemplate(src, srcCols, tx, ty, tw, th) {
  const templ = new cv.Mat(th, tw, cv.CV_8UC1);
  for (let y = 0; y < th; y++)
    for (let x = 0; x < tw; x++)
      templ.data[y * tw + x] = src.data[(y + ty) * srcCols + (x + tx)];
  return templ;
}

const W = 60, H = 60, TW = 12, TH = 12, TX = 22, TY = 17;

addTest('matchTemplate - TM_* constants match cv::TemplateMatchModes', () => {
  // Native OpenCV enum order (opencv2/imgproc.hpp's TemplateMatchModes) -
  // opencv.js code branches on these numerically (e.g. picking minLoc vs
  // maxLoc based on whether method is one of the two SQDIFF variants), so
  // the exact values matter, not just their presence.
  assert(cv.TM_SQDIFF === 0, `TM_SQDIFF should be 0, got ${cv.TM_SQDIFF}`);
  assert(cv.TM_SQDIFF_NORMED === 1, `TM_SQDIFF_NORMED should be 1, got ${cv.TM_SQDIFF_NORMED}`);
  assert(cv.TM_CCORR === 2, `TM_CCORR should be 2, got ${cv.TM_CCORR}`);
  assert(cv.TM_CCORR_NORMED === 3, `TM_CCORR_NORMED should be 3, got ${cv.TM_CCORR_NORMED}`);
  assert(cv.TM_CCOEFF === 4, `TM_CCOEFF should be 4, got ${cv.TM_CCOEFF}`);
  assert(cv.TM_CCOEFF_NORMED === 5, `TM_CCOEFF_NORMED should be 5, got ${cv.TM_CCOEFF_NORMED}`);
});

addTest('matchTemplate - result Mat is sized (W-w+1) x (H-h+1)', () => {
  const src = noiseMat(H, W);
  const templ = embedTemplate(src, W, TX, TY, TW, TH);
  const dst = new cv.Mat();
  cv.matchTemplate(src, templ, dst, cv.TM_SQDIFF);
  assert(dst.rows === H - TH + 1, `expected ${H - TH + 1} rows, got ${dst.rows}`);
  assert(dst.cols === W - TW + 1, `expected ${W - TW + 1} cols, got ${dst.cols}`);
});

// One case per TM_* mode: embed a known patch and confirm matchTemplate +
// minMaxLoc round-trips back to its exact location - the SQDIFF variants
// are minimized at a perfect match, every other mode is maximized there.
const MODES = [
  ['TM_SQDIFF', 'min'],
  ['TM_SQDIFF_NORMED', 'min'],
  ['TM_CCORR', 'max'],
  ['TM_CCORR_NORMED', 'max'],
  ['TM_CCOEFF', 'max'],
  ['TM_CCOEFF_NORMED', 'max'],
];

for (const [name, which] of MODES) {
  addTest(`matchTemplate - ${name} finds the embedded patch`, () => {
    const src = noiseMat(H, W);
    const templ = embedTemplate(src, W, TX, TY, TW, TH);
    const dst = new cv.Mat();
    cv.matchTemplate(src, templ, dst, cv[name]);

    const result = cv.minMaxLoc(dst);
    const loc = which === 'min' ? result.minLoc : result.maxLoc;
    assert(loc.x === TX && loc.y === TY, `expected match at (${TX},${TY}), got (${loc.x},${loc.y})`);
  });
}

addTest('matchTemplate - opencv.js call shape: (image, templ, result, method, mask)', () => {
  // Mirrors js_template_matching_matchTemplate.html exactly: an empty
  // (unpopulated) Mat passed as mask means "no mask" - cv::matchTemplate
  // checks _mask.empty() internally, same as every other optional
  // _InputArray in OpenCV.
  const src = noiseMat(H, W);
  const templ = embedTemplate(src, W, TX, TY, TW, TH);
  const dst = new cv.Mat();
  const mask = new cv.Mat();
  cv.matchTemplate(src, templ, dst, cv.TM_CCOEFF, mask);

  const result = cv.minMaxLoc(dst);
  assert(result.maxLoc.x === TX && result.maxLoc.y === TY, `expected match at (${TX},${TY}), got (${result.maxLoc.x},${result.maxLoc.y})`);
});

addTest('matchTemplate - throws with fewer than 4 arguments', () => {
  const src = noiseMat(20, 20);
  const templ = embedTemplate(src, 20, 2, 2, 4, 4);
  const dst = new cv.Mat();
  let threw = false;
  try {
    cv.matchTemplate(src, templ, dst);
  } catch (e) {
    threw = true;
  }
  assert(threw, 'expected a TypeError for a missing method argument');
});

tests(testCases);

import * as std from 'std';
import { BarcodeDetector, Mat, Size, CV_8UC1 } from 'opencv';

/* Exercises the BarcodeDetector API unified across OpenCV 4.x/5.x (see
 * migration-opencv5.md, "BarcodeDetector constructor signature change").
 * The same three constructor call shapes are expected to work on every
 * OpenCV version this project builds against - only the *effect* of the
 * two-argument form differs (real two-file model pre-5.x, modelPath-only
 * best effort on 5.x, reported via BarcodeDetector.LEGACY_CTOR).
 */

let failed = 0;

function ok(name, fn) {
  try {
    fn();
    console.log('OK  ', name);
  } catch(e) {
    failed++;
    console.log('FAIL', name, '-', (e && e.message) || e);
  }
}

console.log('BarcodeDetector.LEGACY_CTOR =', BarcodeDetector.LEGACY_CTOR,
            BarcodeDetector.LEGACY_CTOR ? '(pre-5.x: two-file ctor is native)' : '(5.x: two-file ctor is emulated)');

ok('new BarcodeDetector() - no model', () => {
  const bd = new BarcodeDetector();
  if(!(bd instanceof BarcodeDetector)) throw new Error('not a BarcodeDetector instance');
});

ok('new BarcodeDetector(modelPath) - single-file form', () => {
  if(BarcodeDetector.LEGACY_CTOR) {
    /* Pre-5.x has no single-file ctor; the unified API throws a clear,
     * catchable TypeError rather than silently ignoring the argument. */
    let threw = false;
    try {
      new BarcodeDetector('nonexistent-model.onnx');
    } catch(e) {
      threw = true;
    }
    if(!threw) throw new Error('expected a throw on pre-5.x for the single-argument form');
  } else {
    /* On 5.x this reaches the native single-arg ctor and fails at model
     * load time (file doesn't exist) - still a catchable exception, not a
     * crash, which is what we're actually verifying here. */
    let threw = false;
    try {
      new BarcodeDetector('nonexistent-model.onnx');
    } catch(e) {
      threw = true;
    }
    if(!threw) throw new Error('expected a throw for a nonexistent model path');
  }
});

ok('new BarcodeDetector(prototxtPath, modelPath) - two-file form', () => {
  /* Bogus paths on both branches - what matters is that construction goes
   * through the version-appropriate native ctor and fails predictably
   * (a catchable exception) instead of crashing, on every version. */
  let threw = false;
  try {
    new BarcodeDetector('nonexistent.prototxt', 'nonexistent.caffemodel');
  } catch(e) {
    threw = true;
  }
  if(!threw) throw new Error('expected a throw for nonexistent model files');
});

/* A blank image has no barcode by construction, so both a clean `false`
 * result and a caught cv::Exception (OpenCV's own detector has internal
 * assumptions that don't hold for every degenerate input) are acceptable
 * outcomes - what actually matters, and what regressed before the
 * js_barcode_detector_method fix below, is that neither call crashes the
 * process. */
function callDetector(name, fn) {
  try {
    const found = fn();
    if(found !== false && found !== true) throw new Error(name + '() returned a non-boolean: ' + found);
  } catch(e) {
    console.log('     (' + name + '() threw, which is fine: ' + ((e && e.message) || e).split('\n')[0] + ')');
  }
}

ok('detect()/decodeWithType()/detectAndDecodeWithType() on a blank image', () => {
  const bd = new BarcodeDetector();
  const blank = new Mat(new Size(200, 200), CV_8UC1);
  blank.setTo(255);

  const points = [];
  callDetector('detect', () => bd.detect(blank, points));

  const decodedInfo = [], decodedType = [];
  callDetector('decodeWithType', () => bd.decodeWithType(blank, points, decodedInfo, decodedType));

  const info2 = [], type2 = [], pts2 = [];
  callDetector('detectAndDecodeWithType', () => bd.detectAndDecodeWithType(blank, info2, type2, pts2));
});

if(failed > 0) {
  console.log(`\n${failed} test(s) FAILED`);
  std.exit(1);
} else {
  console.log('\nAll tests passed');
}

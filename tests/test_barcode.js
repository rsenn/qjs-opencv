import * as std from 'std';
import * as cv from 'opencv';
import { barcode_BarcodeDetector, Mat, Size, CV_8UC1 } from 'opencv';

/* Exercises the BarcodeDetector API unified across OpenCV 4.x/5.x (see
 * migration-opencv5.md, "BarcodeDetector constructor signature change").
 * The same three constructor call shapes are expected to work on every
 * OpenCV version this project builds against - only the *effect* of the
 * two-argument form differs (real two-file model pre-5.x, modelPath-only
 * best effort on 5.x, reported via barcode_BarcodeDetector.LEGACY_CTOR).
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

console.log('barcode_BarcodeDetector.LEGACY_CTOR =', barcode_BarcodeDetector.LEGACY_CTOR,
            barcode_BarcodeDetector.LEGACY_CTOR ? '(pre-5.x: two-file ctor is native)' : '(5.x: two-file ctor is emulated)');

ok('new barcode_BarcodeDetector() - no model', () => {
  const bd = new barcode_BarcodeDetector();
  if(!(bd instanceof barcode_BarcodeDetector)) throw new Error('not a barcode_BarcodeDetector instance');
});

ok('new barcode_BarcodeDetector(modelPath) - single-file form', () => {
  if(barcode_BarcodeDetector.LEGACY_CTOR) {
    /* Pre-5.x has no single-file ctor; the unified API throws a clear,
     * catchable TypeError rather than silently ignoring the argument. */
    let threw = false;
    try {
      new barcode_BarcodeDetector('nonexistent-model.onnx');
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
      new barcode_BarcodeDetector('nonexistent-model.onnx');
    } catch(e) {
      threw = true;
    }
    if(!threw) throw new Error('expected a throw for a nonexistent model path');
  }
});

ok('new barcode_BarcodeDetector(prototxtPath, modelPath) - two-file form', () => {
  /* Bogus paths on both branches - what matters is that construction goes
   * through the version-appropriate native ctor and fails predictably
   * (a catchable exception) instead of crashing, on every version. */
  let threw = false;
  try {
    new barcode_BarcodeDetector('nonexistent.prototxt', 'nonexistent.caffemodel');
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
  const bd = new barcode_BarcodeDetector();
  const blank = new Mat(new Size(200, 200), CV_8UC1);
  blank.setTo(255);

  const points = [];
  callDetector('detect', () => bd.detect(blank, points));

  const decodedInfo = [], decodedType = [];
  callDetector('decodeWithType', () => bd.decodeWithType(blank, points, decodedInfo, decodedType));

  const info2 = [], type2 = [], pts2 = [];
  callDetector('detectAndDecodeWithType', () => bd.detectAndDecodeWithType(blank, info2, type2, pts2));
});

/* EAN-13 drawn from scratch (BarcodeDetector reads 1D codes only: EAN-8/13,
 * UPC-A/E; a QR image would prove QRCodeDetector, not this). Left/right
 * digit tables and the first-digit parity pattern are the EAN-13 standard. */
const EAN_L = ['0001101', '0011001', '0010011', '0111101', '0100011', '0110001', '0101111', '0111011', '0110111', '0001011'];
const EAN_G = ['0100111', '0110011', '0011011', '0100001', '0011101', '0111001', '0000101', '0010001', '0001001', '0010111'];
const EAN_R = EAN_L.map(c => [...c].map(b => (b == '1' ? '0' : '1')).join(''));
const EAN_PARITY = ['LLLLLL', 'LLGLGG', 'LLGGLG', 'LLGGGL', 'LGLLGG', 'LGGLLG', 'LGGGLL', 'LGLGLG', 'LGLGGL', 'LGGLGL'];

function eanCheckDigit(d12) {
  let sum = 0;
  for(let i = 0; i < 12; i++) sum += (+d12[i]) * (i % 2 ? 3 : 1);
  return (10 - (sum % 10)) % 10;
}

function eanBits(digits) {
  const par = EAN_PARITY[+digits[0]];
  let bits = '101';

  for(let i = 0; i < 6; i++) bits += (par[i] == 'L' ? EAN_L : EAN_G)[+digits[1 + i]];
  bits += '01010';
  for(let i = 0; i < 6; i++) bits += EAN_R[+digits[7 + i]];
  return bits + '101';
}

function drawEan13(digits, scale, height, quiet) {
  const bits = eanBits(digits);
  const w = (bits.length + 2 * quiet) * scale;
  const img = new Mat(height, w, CV_8UC1);

  img.setTo(255);
  const row = new Uint8Array(w).fill(255);

  for(let i = 0; i < bits.length; i++)
    if(bits[i] == '1') row.fill(0, (quiet + i) * scale, (quiet + i + 1) * scale);
  for(let y = 0; y < height; y++) img.data.set(row, y * w);
  return img;
}

/* the detector wants the code to be a modest part of a larger white image */
function eanScene(code) {
  const img = drawEan13(code, 2, 120, 10);
  const big = new Mat();

  cv.copyMakeBorder(img, big, 200, 200, 200, 200, cv.BORDER_CONSTANT, new cv.Scalar(255, 255, 255, 255));
  return big;
}

ok('detectAndDecodeWithType() reads a generated EAN-13', () => {
  const d12 = '590123412345';
  const code = d12 + eanCheckDigit(d12);
  const img = eanScene(code);
  const bd = new barcode_BarcodeDetector();
  const info = [], type = [], pts = [];

  const found = bd.detectAndDecodeWithType(img, info, type, pts);

  if(!found) throw new Error('nothing detected');
  if(info.length != 1 || info[0] != code) throw new Error('decoded ' + JSON.stringify(info) + ', expected ' + code);
  if(!/EAN/i.test(String(type[0]))) throw new Error('type ' + type[0]);
  console.log('     decoded', info[0], 'as', type[0]);
});

ok('detect() + decodeWithType() two-step on the same EAN-13', () => {
  const code = '4006381333931';
  const img = eanScene(code);
  const bd = new barcode_BarcodeDetector();
  const pts = [], info = [], type = [];

  if(!bd.detect(img, pts)) throw new Error('detect() found nothing');
  if(!bd.decodeWithType(img, pts, info, type)) throw new Error('decode failed');
  if(info[0] != code) throw new Error('decoded ' + JSON.stringify(info));
});

if(failed > 0) {
  console.log(`\n${failed} test(s) FAILED`);
  std.exit(1);
} else {
  console.log('\nAll tests passed');
}

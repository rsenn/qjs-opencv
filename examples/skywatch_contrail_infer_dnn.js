/**
 * Inference stub for phase 6's learned contrail detector - see
 * js/skywatch/skywatch.md's "Phase 6" section for the model (small U-Net,
 * single-channel whiteness in, per-pixel probability out) and
 * js/skywatch/train_contrail_unet.py for how the .onnx this expects gets
 * produced. Not runnable today: there is no trained model until a real
 * dataset exists (js/skywatch/export-training-example.js) and training has
 * actually been run. This is scaffolding to drop that model into, written
 * now so it isn't designed from scratch under time pressure later -
 * follows the same cv.dnn.readNetFromONNX pattern as
 * examples/edge_detection_dexined.js.
 *
 * Usage: qjsm examples/skywatch_contrail_infer_dnn.js <input.jpg> <output.jpg> [model.onnx]
 */
import * as cv from 'opencv';
import { buildCandidateMask } from '../js/skywatch/detect.js';

const DEFAULT_MODEL = 'js/skywatch/models/contrail_unet.onnx';
const PROB_THRESHOLD = 0.5;

// U-Net has 3 pooling steps (see train_contrail_unet.py's ContrailUNet),
// so H/W must be multiples of 8 for the encoder/decoder skip connections
// to line up - pad up rather than resize, so we don't distort the thin
// trail geometry the model is trying to localize precisely.
function padToMultiple(mat, multiple) {
  const h = mat.rows, w = mat.cols;
  const padH = (multiple - (h % multiple)) % multiple;
  const padW = (multiple - (w % multiple)) % multiple;
  if (padH === 0 && padW === 0) return { padded: mat, padH, padW };
  const padded = new cv.Mat();
  cv.copyMakeBorder(mat, padded, 0, padH, 0, padW, cv.BORDER_REPLICATE);
  return { padded, padH, padW };
}

function runModel(net, whiteness) {
  const { padded, padH, padW } = padToMultiple(whiteness, 8);

  // Single-channel input, [0,1] range (matches ContrailDataset's
  // `/ 255.0` normalization in train_contrail_unet.py) - blobFromImage's
  // scalefactor does that division, mean=0, no channel swap needed for a
  // 1-channel image.
  const blob = cv.blobFromImage(padded, 1.0 / 255.0, new cv.Size(padded.cols, padded.rows), [0], false, false);
  net.setInput(blob);
  const out = net.forward(); // (1,1,H,W) probability map

  const probMat = cv.matFromArray(padded.rows, padded.cols, cv.CV_32FC1, out.data32F);
  const cropped = padH || padW
    ? probMat(new cv.Rect(0, 0, whiteness.cols, whiteness.rows))
    : probMat;

  const mask = new cv.Mat();
  cv.threshold(cropped, mask, PROB_THRESHOLD, 255, cv.THRESH_BINARY);
  mask.convertTo(mask, cv.CV_8UC1);
  return mask;
}

function main(inputPath, outputPath, modelPath = DEFAULT_MODEL) {
  const bgr = cv.imread(inputPath);
  if (bgr.empty()) throw new Error(`could not read ${inputPath}`);

  const hsv = new cv.Mat();
  cv.cvtColor(bgr, hsv, cv.COLOR_BGR2HSV);
  const { whiteness } = buildCandidateMask(hsv);

  const net = cv.readNetFromONNX(modelPath);
  const mask = runModel(net, whiteness);

  const output = bgr.clone();
  output.setTo(new cv.Scalar(0, 0, 255), mask);
  cv.imwrite(outputPath, output);
  console.log(JSON.stringify({ input: inputPath, model: modelPath, contrailPixels: cv.countNonZero(mask) }));
}

main(...scriptArgs.slice(1));

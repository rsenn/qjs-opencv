/* Neural Font Super-Resolution (FSRCNN x2) via OpenCV 5 DNN & ONNX
 * 
 * Reconstructs clean, high-precision font contours and strips stray pixels.
 * Model: FSRCNN x2 (Fast Super-Resolution Convolutional Neural Network)
 * 
 * Usage: qjsm examples/neural_font_sr_fsrcnn.js <input.png> [output.png]
 */
import * as cv from 'opencv';

const MODEL = 'examples/models/super_res_fsrcnn/fsrcnn_x2.onnx';

function upscaleFontGlyphs(net, image) {
  // Ensure we operate cleanly within a bounds-checked ROI
  const maxW = 224;
  const maxH = 224;
  
  const w = Math.min(image.cols, maxW);
  const h = Math.min(image.rows, maxH);
  
  const sub = image(new cv.Rect(0, 0, w, h));
  const padded = cv.Mat.zeros(maxH, maxW, image.type());
  const roi = padded(new cv.Rect(0, 0, w, h));
  sub.copyTo(roi);

  // FSRCNN takes standard 3-channel or 1-channel blobs scaled to [0, 1]
  const blob = cv.blobFromImage(
    padded,
    1.0 / 255.0,
    new cv.Size(padded.cols, padded.rows),
    [0, 0, 0],
    /*swapRB*/ false,
    /*crop*/ false
  );

  // blobFromImage always shapes its output NCHW (1,1,H,W); this
  // particular ONNX graph (converted from the original TensorFlow
  // FSRCNN model, which is NHWC-native) declares its input as (1,H,W,1)
  // instead. With a single channel the underlying byte layout is
  // identical either way (the channel dim is a singleton in both), so
  // this is a pure shape-metadata relabel, not a data copy/transpose.
  const reshaped = blob.reshape(1, [1, padded.rows, padded.cols, 1]);

  net.setInput(reshaped);
  const out = net.forward();

  const dims = out.size();
  const outChannels = dims[1];
  const outHeight = dims[2];
  const outWidth = dims[3];

  const srcData = out.data32F;
  const n = srcData.length;
  const u8Arr = new Float64Array(n);

  for (let i = 0; i < n; i++) {
    const val = Math.min(Math.max(srcData[i], 0.0), 1.0);
    u8Arr[i] = val * 255;
  }

  if (outChannels === 1) {
    return cv.matFromArray(outHeight, outWidth, cv.CV_8UC1, u8Arr);
  } else {
    return cv.matFromArray(outHeight, outWidth, cv.CV_8UC3, u8Arr);
  }
}

function cleanAndDownscale1Bit(upscaledMat, scaleBack = 0.5) {
  const targetWidth = Math.floor(upscaledMat.cols * scaleBack);
  const targetHeight = Math.floor(upscaledMat.rows * scaleBack);

  const downscaled = new cv.Mat();
  // Area interpolation smoothly blends sub-pixel density gradients back down
  cv.resize(upscaledMat, downscaled, new cv.Size(targetWidth, targetHeight), 0, 0, cv.INTER_AREA);

  const gray = new cv.Mat();
  if (downscaled.channels() > 1) {
    cv.cvtColor(downscaled, gray, cv.COLOR_BGR2GRAY);
  } else {
    downscaled.copyTo(gray);
  }

  // Force clean 1-bit binarization via Otsu thresholding
  const binary1Bit = new cv.Mat();
  cv.threshold(gray, binary1Bit, 0, 255, cv.THRESH_BINARY | cv.THRESH_OTSU);

  // Morphological Opening (Erosion followed by Dilation) cleanly eliminates 
  // isolated single-pixel standouts and noise floating near rounded contours.
  const kernel = cv.getStructuringElement(cv.MORPH_RECT, new cv.Size(2, 2));
  const cleaned = new cv.Mat();
  cv.morphologyEx(binary1Bit, cleaned, cv.MORPH_OPEN, kernel);

  return cleaned;
}

function main(input = 'Diamonaire.png', output = 'font_cleaned_fsrcnn.png') {
  const src4 = cv.imread(input);
  if (src4.empty) throw new Error(`Error opening image: ${input}`);

  const src = new cv.Mat();
  if (src4.channels() === 1) {
    src4.copyTo(src);
  } else {
    cv.cvtColor(src4, src, src4.channels() === 4 ? cv.COLOR_BGRA2GRAY : cv.COLOR_BGR2GRAY);
  }

  console.log(`Loading FSRCNN ONNX model from ${MODEL}...`);
  const net = cv.readNetFromONNX(MODEL);

  console.log(`Running FSRCNN 2x Super-Resolution inference...`);
  const upscaled = upscaleFontGlyphs(net, src);

  const result = cleanAndDownscale1Bit(upscaled, 0.5);

  cv.imwrite(output, result);
  console.log(`Successfully wrote cleaned 1-bit font map to ${output}`);
}

main(...scriptArgs.slice(1));

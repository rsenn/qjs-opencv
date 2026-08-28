/* Neural Font Super-Resolution (ESPCN x2) via OpenCV 5 DNN & ONNX
 *
 * Reconstructs clean, double-resolution text glyphs from blocky low-res pixel art.
 * Model: ESPCN x2 (Efficient Sub-Pixel Convolutional Network)
 *
 * Usage: qjsm examples/font_super_resolution_espcn.js <input.png> [output.png]
 */
import * as cv from 'opencv';

const MODEL = 'examples/models/super_res_espcn/espcn_x2.onnx';
const SCALE_FACTOR = 2;

function upscaleFontGlyphs(net, image) {
  image = image(new cv.Rect(0, 0, 224, 224));

  const padded = cv.Mat.zeros(224, 224, image.type());
  const roi = padded(new cv.Rect(0, 0, image.cols, image.rows));
  image.copyTo(roi);

  cv.imwrite('padded.png', padded);

  console.log('padded', padded.size() + '');

  // ESPCN expects normalized Y/grayscale or luminance channels,
  // but can process standard BGR blobs scaled to [0, 1].
  const blob = cv.blobFromImage(padded, 1.0 / 255.0, new cv.Size(padded.cols, padded.rows), [0, 0, 0], /*swapRB*/ false, /*crop*/ false);

  net.setInput(blob);
  const out = net.forward();

  /* The output tensor shape from ESPCN is typically (1, 3, H*scale, W*scale) or (1, 1, H*scale, W*scale).
   * We extract and reshape the tensor data via flat Float32Array views and matFromArray. */
  const dims = out.size(); // e.g. [1, 3, height * 2, width * 2]
  /*console.log('out',   out);
 console.log('out.size()',   out.size());
 console.log('out',   out);*/
  const outChannels = dims[1];
  const outHeight = dims[2];
  const outWidth = dims[3];

  // For visual clarity on font maps, flatten/convert back to 8U format
  const srcData = out.data32F;

  console.log('srcData', srcData);

  const n = srcData.length;
  const u8Arr = new Float64Array(n);

  for(let i = 0; i < n; i++) {
    // Clamp values between 0.0 and 1.0, scale to 255
    const val = Math.min(Math.max(srcData[i], 0.0), 1.0);
    u8Arr[i] = val * 255;
  }

  // Handle channel reconstruction depending on output format
  if(outChannels === 1) {
    return cv.matFromArray(outHeight, outWidth, cv.CV_8UC1, u8Arr);
  } else {
    // If 3-channel color output, build a multi-channel Mat
    // (Simplified here by extracting the first channel or handling interleaving)
    return cv.matFromArray(outHeight, outWidth, cv.CV_8UC3, u8Arr);
  }
}

function main(input = 'tests/glyph_map.png', output = 'font_upscaled_espcn.png') {
  const src4 = cv.imread(input);

  if(src4.empty) throw new Error(`Error opening image: ${input}`);

  // Ensure 3-channel BGR input
  const src = new cv.Mat();
  if(src4.channels() == 1) src4.copyTo(src);
  else cv.cvtColor(src4, src, src4.channels() === 4 ? cv.COLOR_BGRA2GRAY : cv.COLOR_BGR2GRAY);

  console.log(`Loading ONNX model from ${MODEL}...`);
  const net = cv.readNetFromONNX(MODEL);

  console.log(`Running 2x Super-Resolution inference on ${src.cols}x${src.rows} glyph map...`);
  const upscaled = upscaleFontGlyphs(net, src);

  cv.threshold(upscaled, upscaled, 127, 255, cv.THRESH_BINARY);

  cv.imwrite(output, upscaled);
  cv.imshow('upscaled', upscaled);
  cv.waitKey(-1);
  console.log(`Successfully wrote upscaled font map to ${output}`);
}

main(...scriptArgs.slice(1));

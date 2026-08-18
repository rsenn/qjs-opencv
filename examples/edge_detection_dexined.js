/* Learned (DNN) edge detection, DexiNed-style — see TODO.md "dnn — high-level
 * Model wrappers for learned edge detection". Produces a cleaner,
 * semantically-aware edge map than Canny for feeding into
 * findContours/vectorization on complex photographs.
 *
 * Model: examples/models/edge_detection_dexined/edge_detection_dexined_2024sep.onnx
 * (fetched from https://github.com/opencv/opencv_zoo, sha1
 * f86f2d32c3cf892771f76b5e6b629b16a66510e9 — matches OpenCV's own
 * samples/dnn/models.yml entry for the "dexined" edge_detection sample).
 *
 * Usage: qjsm examples/edge_detection_dexined.js <input.png> [output.png]
 */
import * as cv from 'opencv';

const MODEL = 'examples/models/edge_detection_dexined/edge_detection_dexined_2024sep.onnx';
const NET_SIZE = new cv.Size(512, 512);
const MEAN = [103.5, 116.2, 123.6];

/* The network's forward() output is a 4D (1,1,512,512) blob, so it has no
 * usable .rows/.cols (both -1) - reshape via a flat sigmoid + min-max stretch
 * over its .data32F view, then hand the result to matFromArray to get a
 * proper 2D Mat back. */
function sigmoidStretchTo8U(out, side) {
  const src = out.data32F;
  const n = src.length;
  const sig = new Float64Array(n);
  let min = Infinity, max = -Infinity;

  for(let i = 0; i < n; i++) {
    const v = 1.0 / (1.0 + Math.exp(-src[i]));
    sig[i] = v;
    if(v < min) min = v;
    if(v > max) max = v;
  }

  const range = max - min || 1;
  const u8 = new Float64Array(n);

  for(let i = 0; i < n; i++) u8[i] = ((sig[i] - min) / range) * 255;

  return cv.matFromArray(side, side, cv.CV_8UC1, u8);
}

function detectEdges(net, image) {
  const blob = cv.blobFromImage(image, 1.0, NET_SIZE, MEAN, /*swapRB*/ false, /*crop*/ false);

  net.setInput(blob);

  const out = net.forward();
  const edges8u = sigmoidStretchTo8U(out, NET_SIZE.width);

  const resized = new cv.Mat();
  cv.resize(edges8u, resized, new cv.Size(image.cols, image.rows));

  return resized;
}

function main(input = 'tests/smarties.png', output = 'edge_detection_dexined.png') {
  const src4 = cv.imread(input);

  if(src4.empty) throw new Error(`Error opening image: ${input}`);

  const src = new cv.Mat();
  cv.cvtColor(src4, src, src4.channels() === 4 ? cv.COLOR_BGRA2BGR : cv.COLOR_GRAY2BGR);

  const net = cv.readNetFromONNX(MODEL);
  const edges = detectEdges(net, src);

  cv.imwrite(output, edges);
  console.log(`wrote ${output}`);
}

main(...scriptArgs.slice(1));

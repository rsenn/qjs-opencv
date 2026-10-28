import { tests, assert, eq } from './tinytest.js';
import * as cv from 'opencv';
import * as fs from 'fs';

// net.forward() call forms. Needs models/u2netp.onnx (not committed,
// see doc/HANDOFF-imgproc-models.md); skipped when it is missing.

const MODEL = import.meta.url.replace(/^file:\/\//, '').replace(/[^/]*\/[^/]*\/[^/]*$/, 'models/u2netp.onnx');
const have = fs.existsSync(MODEL);

tests({
  'forward(names) returns an array of Mats'() {
    if(!have) return print('  skipped: ' + MODEL + ' missing');
    const net = cv.readNetFromONNX(MODEL);
    const img = new cv.Mat(320, 320, cv.CV_8UC3, new cv.Scalar(90, 120, 200));
    net.setInput(cv.blobFromImage(img, 0.019, new cv.Size(320, 320), new cv.Scalar(123.6, 116.2, 103.5), true, false));
    const names = net.getUnconnectedOutLayersNames();
    const out = net.forward(names);
    assert(Array.isArray(out), 'array');
    eq(out.length, names.length);
    assert(out[0] instanceof cv.Mat, 'Mat');
    eq(JSON.stringify(out[0].size()), '[1,1,320,320]');
  },
  'forward(name) and forward(names) agree'() {
    if(!have) return print('  skipped: ' + MODEL + ' missing');
    const net = cv.readNetFromONNX(MODEL);
    const img = new cv.Mat(320, 320, cv.CV_8UC3, new cv.Scalar(90, 120, 200));
    net.setInput(cv.blobFromImage(img, 0.019, new cv.Size(320, 320), new cv.Scalar(123.6, 116.2, 103.5), true, false));
    const names = net.getUnconnectedOutLayersNames();
    const a = net.forward(names)[0].data32F, b = net.forward(names[0]).data32F;
    eq(a.length, b.length);
    eq(a[1000], b[1000]);
  },
  'forward(outArray, names) still fills the array'() {
    if(!have) return print('  skipped: ' + MODEL + ' missing');
    const net = cv.readNetFromONNX(MODEL);
    const img = new cv.Mat(320, 320, cv.CV_8UC3, new cv.Scalar(90, 120, 200));
    net.setInput(cv.blobFromImage(img, 0.019, new cv.Size(320, 320), new cv.Scalar(123.6, 116.2, 103.5), true, false));
    const names = net.getUnconnectedOutLayersNames(), out = [];
    net.forward(out, names);
    eq(out.length, names.length);
  },
});

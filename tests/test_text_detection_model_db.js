import * as std from 'std';
import { dnn, Mat, Size, CV_8UC3 } from 'opencv';

/* cv::dnn::TextDetectionModel_DB - see tests/test_model.js for why this
 * only exercises the JS binding surface (no real weights available here).
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

ok('new TextDetectionModel_DB(net) from an empty dnn.Net', () => {
  const model = new dnn.TextDetectionModel_DB(new dnn.Net());
  if(!(model instanceof dnn.TextDetectionModel_DB)) throw new Error('not a TextDetectionModel_DB instance');
});

ok('new TextDetectionModel_DB(modelPath) with a bogus path throws, not crashes', () => {
  let threw = false;
  try {
    new dnn.TextDetectionModel_DB('nonexistent-model.onnx');
  } catch(e) {
    threw = true;
  }
  if(!threw) throw new Error('expected a throw for a nonexistent model path');
});

ok('setBinaryThreshold()/getBinaryThreshold() round-trip', () => {
  const model = new dnn.TextDetectionModel_DB(new dnn.Net());
  model.setBinaryThreshold(0.3);
  if(Math.abs(model.getBinaryThreshold() - 0.3) > 1e-5) throw new Error('did not round-trip');
});

ok('setPolygonThreshold()/getPolygonThreshold() round-trip', () => {
  const model = new dnn.TextDetectionModel_DB(new dnn.Net());
  model.setPolygonThreshold(0.5);
  if(Math.abs(model.getPolygonThreshold() - 0.5) > 1e-5) throw new Error('did not round-trip');
});

ok('setUnclipRatio()/getUnclipRatio() round-trip', () => {
  const model = new dnn.TextDetectionModel_DB(new dnn.Net());
  model.setUnclipRatio(2.0);
  if(Math.abs(model.getUnclipRatio() - 2.0) > 1e-5) throw new Error('did not round-trip');
});

ok('setMaxCandidates()/getMaxCandidates() round-trip', () => {
  const model = new dnn.TextDetectionModel_DB(new dnn.Net());
  model.setMaxCandidates(100);
  if(model.getMaxCandidates() !== 100) throw new Error('did not round-trip');
});

ok('detect()/detectTextRectangles() on an empty network throw catchable exceptions', () => {
  const model = new dnn.TextDetectionModel_DB(new dnn.Net());
  model.setInputSize(1, 1);

  const frame = new Mat(new Size(4, 4), CV_8UC3);
  frame.setTo([0, 0, 0]);

  let threw = 0;
  try {
    model.detect(frame, []);
  } catch(e) {
    threw++;
  }
  try {
    model.detectTextRectangles(frame, []);
  } catch(e) {
    threw++;
  }
  if(threw !== 2) throw new Error('expected both detect() and detectTextRectangles() to throw, got ' + threw);
});

if(failed > 0) {
  console.log(`\n${failed} test(s) FAILED`);
  std.exit(1);
} else {
  console.log('\nAll tests passed');
}

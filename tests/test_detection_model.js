import * as std from 'std';
import { dnn, Mat, Size, CV_8UC3 } from 'opencv';

/* cv::dnn::DetectionModel - see tests/test_model.js for why this only
 * exercises the JS binding surface (no real weights available here).
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

ok('new DetectionModel(net) from an empty dnn.Net', () => {
  const model = new dnn.DetectionModel(new dnn.Net());
  if(!(model instanceof dnn.DetectionModel)) throw new Error('not a DetectionModel instance');
});

ok('new DetectionModel(modelPath) with a bogus path throws, not crashes', () => {
  let threw = false;
  try {
    new dnn.DetectionModel('nonexistent-model.onnx');
  } catch(e) {
    threw = true;
  }
  if(!threw) throw new Error('expected a throw for a nonexistent model path');
});

ok('setNmsAcrossClasses()/getNmsAcrossClasses() round-trip', () => {
  const model = new dnn.DetectionModel(new dnn.Net());
  model.setNmsAcrossClasses(true);
  if(model.getNmsAcrossClasses() !== true) throw new Error('expected true after enabling');
  model.setNmsAcrossClasses(false);
  if(model.getNmsAcrossClasses() !== false) throw new Error('expected false after disabling');
});

ok('detect() on an empty network throws a catchable exception', () => {
  const model = new dnn.DetectionModel(new dnn.Net());
  model.setInputSize(1, 1);

  const frame = new Mat(new Size(4, 4), CV_8UC3);
  frame.setTo([0, 0, 0]);

  const classIds = [], confidences = [], boxes = [];
  let threw = false;
  try {
    model.detect(frame, classIds, confidences, boxes, 0.5, 0.0);
  } catch(e) {
    threw = true;
  }
  if(!threw) throw new Error('expected detect() on an empty network to throw');
});

if(failed > 0) {
  console.log(`\n${failed} test(s) FAILED`);
  std.exit(1);
} else {
  console.log('\nAll tests passed');
}

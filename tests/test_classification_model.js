import * as std from 'std';
import { dnn, Mat, Size, CV_8UC3 } from 'opencv';

/* cv::dnn::ClassificationModel - see tests/test_model.js for why this only
 * exercises the JS binding surface (no real weights available here), and
 * migration-opencv5.md, "Wrapping the classic DNN convenience Model classes".
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

ok('new ClassificationModel(net) from an empty dnn.Net', () => {
  const model = new dnn.ClassificationModel(new dnn.Net());
  if(!(model instanceof dnn.ClassificationModel)) throw new Error('not a ClassificationModel instance');
});

ok('new ClassificationModel(modelPath) with a bogus path throws, not crashes', () => {
  let threw = false;
  try {
    new dnn.ClassificationModel('nonexistent-model.onnx');
  } catch(e) {
    threw = true;
  }
  if(!threw) throw new Error('expected a throw for a nonexistent model path');
});

ok('setEnableSoftmaxPostProcessing()/getEnableSoftmaxPostProcessing() round-trip', () => {
  const model = new dnn.ClassificationModel(new dnn.Net());
  model.setEnableSoftmaxPostProcessing(true);
  if(model.getEnableSoftmaxPostProcessing() !== true) throw new Error('expected true after enabling');
  model.setEnableSoftmaxPostProcessing(false);
  if(model.getEnableSoftmaxPostProcessing() !== false) throw new Error('expected false after disabling');
});

ok('classify() on an empty network throws a catchable exception', () => {
  const model = new dnn.ClassificationModel(new dnn.Net());
  model.setInputSize(1, 1);

  const frame = new Mat(new Size(4, 4), CV_8UC3);
  frame.setTo([0, 0, 0]);

  let threw = false;
  try {
    model.classify(frame);
  } catch(e) {
    threw = true;
  }
  if(!threw) throw new Error('expected classify() on an empty network to throw');
});

if(failed > 0) {
  console.log(`\n${failed} test(s) FAILED`);
  std.exit(1);
} else {
  console.log('\nAll tests passed');
}

import * as std from 'std';
import { dnn, Mat, Size, CV_8UC3, CV_8UC1 } from 'opencv';

/* cv::dnn::SegmentationModel - see tests/test_model.js for why this only
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

ok('new SegmentationModel(net) from an empty dnn.Net', () => {
  const model = new dnn.SegmentationModel(new dnn.Net());
  if(!(model instanceof dnn.SegmentationModel)) throw new Error('not a SegmentationModel instance');
});

ok('new SegmentationModel(modelPath) with a bogus path throws, not crashes', () => {
  let threw = false;
  try {
    new dnn.SegmentationModel('nonexistent-model.onnx');
  } catch(e) {
    threw = true;
  }
  if(!threw) throw new Error('expected a throw for a nonexistent model path');
});

ok('segment() on an empty network throws a catchable exception', () => {
  const model = new dnn.SegmentationModel(new dnn.Net());
  model.setInputSize(1, 1);

  const frame = new Mat(new Size(4, 4), CV_8UC3);
  frame.setTo([0, 0, 0]);
  const mask = new Mat();

  let threw = false;
  try {
    model.segment(frame, mask);
  } catch(e) {
    threw = true;
  }
  if(!threw) throw new Error('expected segment() on an empty network to throw');
});

if(failed > 0) {
  console.log(`\n${failed} test(s) FAILED`);
  std.exit(1);
} else {
  console.log('\nAll tests passed');
}

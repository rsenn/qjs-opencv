import * as std from 'std';
import { dnn, Mat, Size, CV_8UC3 } from 'opencv';

/* cv::dnn::Model - the base proxy class every X Model subclass inherits from
 * (setInputSize/Mean/Scale/Crop/SwapRB, setOutputNames, setInputParams,
 * predict, setPreferableBackend/Target, enableWinograd). No real weights are
 * available in this environment, so this exercises the JS binding surface
 * itself (construction shapes, setter/getter safety, predict() on an empty
 * network) rather than real inference - see migration-opencv5.md, "Wrapping
 * LLM inference (Qwen2.5)" / "classic DNN convenience Model classes" for the
 * full picture and why real-model testing isn't feasible here.
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

ok('new Model(net) from an empty dnn.Net', () => {
  const net = new dnn.Net();
  const model = new dnn.Model(net);
  if(!(model instanceof dnn.Model)) throw new Error('not a Model instance');
});

ok('new Model(modelPath) with a bogus path throws, not crashes', () => {
  let threw = false;
  try {
    new dnn.Model('nonexistent-model.onnx');
  } catch(e) {
    threw = true;
  }
  if(!threw) throw new Error('expected a throw for a nonexistent model path');
});

ok('setInputSize/Mean/Scale/Crop/SwapRB/setInputParams are safe on an empty net', () => {
  const model = new dnn.Model(new dnn.Net());
  model.setInputSize(224, 224);
  model.setInputSize(new Size(224, 224));
  model.setInputMean([0, 0, 0]);
  model.setInputScale([1 / 255, 1 / 255, 1 / 255]);
  model.setInputCrop(false);
  model.setInputSwapRB(true);
  model.setInputParams(1 / 255, new Size(224, 224), [0, 0, 0], true, false);
  model.setPreferableBackend(dnn.DNN_BACKEND_OPENCV);
  model.setPreferableTarget(dnn.DNN_TARGET_CPU);
  model.enableWinograd(true);
});

ok('predict() on an empty network throws a catchable exception', () => {
  const model = new dnn.Model(new dnn.Net());
  model.setInputSize(1, 1);

  const frame = new Mat(new Size(4, 4), CV_8UC3);
  frame.setTo([0, 0, 0]);

  const outs = [];
  let threw = false;
  try {
    model.predict(frame, outs);
  } catch(e) {
    threw = true;
  }
  if(!threw) throw new Error('expected predict() on an empty network to throw');
});

if(failed > 0) {
  console.log(`\n${failed} test(s) FAILED`);
  std.exit(1);
} else {
  console.log('\nAll tests passed');
}

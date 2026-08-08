import * as std from 'std';
import { dnn, Mat, Size, CV_8UC3 } from 'opencv';

/* cv::dnn::TextRecognitionModel - see tests/test_model.js for why this only
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

ok('new TextRecognitionModel(net) from an empty dnn.Net', () => {
  const model = new dnn.TextRecognitionModel(new dnn.Net());
  if(!(model instanceof dnn.TextRecognitionModel)) throw new Error('not a TextRecognitionModel instance');
});

ok('new TextRecognitionModel(modelPath) with a bogus path throws, not crashes', () => {
  let threw = false;
  try {
    new dnn.TextRecognitionModel('nonexistent-model.onnx');
  } catch(e) {
    threw = true;
  }
  if(!threw) throw new Error('expected a throw for a nonexistent model path');
});

ok('setDecodeType()/getDecodeType() round-trip', () => {
  const model = new dnn.TextRecognitionModel(new dnn.Net());
  model.setDecodeType('CTC-greedy');
  if(model.getDecodeType() !== 'CTC-greedy') throw new Error('getDecodeType() did not round-trip');
});

ok('setVocabulary()/getVocabulary() round-trip', () => {
  const model = new dnn.TextRecognitionModel(new dnn.Net());
  const vocab = ['a', 'b', 'c'];
  model.setVocabulary(vocab);
  const got = model.getVocabulary();
  if(got.length !== vocab.length || got[0] !== 'a' || got[2] !== 'c')
    throw new Error('getVocabulary() did not round-trip: ' + JSON.stringify(got));
});

ok('setDecodeOptsCTCPrefixBeamSearch() is safe to call', () => {
  const model = new dnn.TextRecognitionModel(new dnn.Net());
  model.setDecodeType('CTC-prefix-beam-search');
  model.setDecodeOptsCTCPrefixBeamSearch(10, 20);
});

ok('recognize() on an empty network throws a catchable exception', () => {
  const model = new dnn.TextRecognitionModel(new dnn.Net());
  model.setInputSize(1, 1);

  const frame = new Mat(new Size(4, 4), CV_8UC3);
  frame.setTo([0, 0, 0]);

  let threw = false;
  try {
    model.recognize(frame);
  } catch(e) {
    threw = true;
  }
  if(!threw) throw new Error('expected recognize() on an empty network to throw');
});

if(failed > 0) {
  console.log(`\n${failed} test(s) FAILED`);
  std.exit(1);
} else {
  console.log('\nAll tests passed');
}

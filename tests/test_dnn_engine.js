import * as std from 'std';
import * as cv from 'opencv';

/* The engine argument of readNetFromONNX: ENGINE_ORT (ONNX Runtime) loads
 * graphs with dynamic shapes that the builtin importer rejects (see
 * doc/dnn-engines.md). Without an ORT-enabled OpenCV only the plumbing is
 * checkable: the constants exist and a bad path throws, not crashes. */
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

ok('ENGINE_* constants are exported on cv.dnn', () => {
  for(const n of ['ENGINE_AUTO', 'ENGINE_OPENCV', 'ENGINE_ORT'])
    if(typeof cv.dnn[n] != 'number') throw new Error(n + ' missing');
});

ok('readNetFromONNX(path, engine) throws on a missing file for every engine', () => {
  for(const e of [cv.dnn.ENGINE_AUTO, cv.dnn.ENGINE_OPENCV, cv.dnn.ENGINE_ORT]) {
    let threw = false;
    try {
      cv.readNetFromONNX('/nonexistent/model.onnx', e);
    } catch(err) {
      threw = true;
    }
    if(!threw) throw new Error('no exception for engine ' + e);
  }
});

if(failed) {
  console.log(`\n${failed} test(s) FAILED`);
  std.exit(1);
}
console.log('\nAll tests passed');

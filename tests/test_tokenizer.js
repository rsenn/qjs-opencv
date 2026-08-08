import * as std from 'std';
import { dnn } from 'opencv';

/* cv::dnn::Tokenizer + Net KV-cache (OpenCV 5.0+ only - see
 * migration-opencv5.md, "Wrapping LLM inference (Qwen2.5)"). Both are
 * undefined/absent on pre-5.x builds (HAVE_OPENCV_DNN_TOKENIZER), which is
 * itself part of what this test verifies.
 *
 * tests/qwen2.5-tokenizer/ ships Qwen2.5's real tokenizer.json (byte-level
 * BPE, downloaded from https://huggingface.co/Qwen/Qwen2.5-0.5B-Instruct)
 * plus a config.json declaring {"model_type": "gpt2"} - Tokenizer::load()'s
 * documented format for a tiktoken/GPT2-style BPE vocabulary, which is what
 * Qwen2.5's tokenizer.json actually contains. As of this writing
 * Tokenizer::load() throws on this fixture (cv::FileStorage internals
 * rejecting it - see the BUGS-adjacent note in migration-opencv5.md); this
 * test documents that as an open compatibility gap rather than asserting a
 * successful load, while still proving the binding itself is safe (a
 * catchable exception, not a crash) either way.
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

const haveTokenizer = typeof dnn.Tokenizer !== 'undefined';
console.log('dnn.Tokenizer =', haveTokenizer ? 'present (OpenCV 5.0+)' : 'absent (pre-5.x, as expected)');

ok('Tokenizer.load() on a bogus directory throws, not crashes', () => {
  if(!haveTokenizer) return; /* nothing to test pre-5.x */

  let threw = false;
  try {
    dnn.Tokenizer.load('/nonexistent/directory/');
  } catch(e) {
    threw = true;
  }
  if(!threw) throw new Error('expected a throw for a nonexistent model directory');
});

ok('Tokenizer.load() on the real Qwen2.5 tokenizer.json fixture does not crash', () => {
  if(!haveTokenizer) return; /* nothing to test pre-5.x */

  const dir = 'qwen2.5-tokenizer/';

  try {
    const tok = dnn.Tokenizer.load(dir);
    const ids = tok.encode('Hello, Qwen2.5!');
    const text = tok.decode(ids);
    console.log('     (loaded ok - encode/decode round-trip:', JSON.stringify(text), ')');
  } catch(e) {
    console.log('     (Tokenizer.load() threw on the real fixture, which is a known open item: ' + ((e && e.message) || e).split('\n')[0] + ')');
  }
});

ok('Net.prototype has enableKVCache/disableKVCache/resetKVCache iff Tokenizer is present', () => {
  const have = typeof dnn.Net.prototype.enableKVCache === 'function'
            && typeof dnn.Net.prototype.disableKVCache === 'function'
            && typeof dnn.Net.prototype.resetKVCache === 'function';

  if(have !== haveTokenizer)
    throw new Error('KV-cache methods and Tokenizer should be gated by the same HAVE_OPENCV_DNN_TOKENIZER flag');
});

ok('disableKVCache() is safe to call on an empty Net', () => {
  if(!haveTokenizer) return; /* not bound pre-5.x */

  /* enableKVCache()/resetKVCache() on a Net with no layers loaded segfault
   * inside OpenCV itself (a native crash, not a catchable cv::Exception) -
   * see BUGS: opencv-net-enablekvcache-segfaults-on-empty-net. Deliberately
   * not exercised here since a real model would need to be loaded first;
   * only disableKVCache(), which is unconditionally safe, is checked. */
  const net = new dnn.Net();
  net.disableKVCache();
});

if(failed > 0) {
  console.log(`\n${failed} test(s) FAILED`);
  std.exit(1);
} else {
  console.log('\nAll tests passed');
}

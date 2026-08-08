import * as std from 'std';
import { dnn } from 'opencv';

/* cv::dnn::Tokenizer + Net KV-cache (OpenCV 5.0+ only - see
 * migration-opencv5.md, "Wrapping LLM inference (Qwen2.5)"). Both are
 * undefined/absent on pre-5.x builds (HAVE_OPENCV_DNN_TOKENIZER), which is
 * itself part of what this test verifies.
 *
 * tests/qwen2.5-tokenizer/ ships Qwen2.5's real tokenizer.json (byte-level
 * BPE, downloaded from https://huggingface.co/Qwen/Qwen2.5-0.5B-Instruct)
 * plus a config.json declaring {"model_type": "qwen2.5"}. Two gotchas found
 * by reading OpenCV's actual tokenizer.cpp (dnn.hpp's doc comment on
 * Tokenizer::load() is misleading on both counts):
 *   - the argument is the path to config.json itself, not a directory - the
 *     tokenizer.json directory is derived by stripping the filename back off
 *     whatever path is given;
 *   - model_type must be "qwen2"/"qwen2.5" for a real Qwen tokenizer.json,
 *     not "gpt2"/"gpt4" - those select a different regex-split pattern and
 *     silently mis-tokenize instead of failing outright.
 * With both fixed, this does a real encode/decode round-trip against
 * Qwen2.5's actual vocabulary, not just a "doesn't crash" check.
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

ok('Tokenizer.load() on a bogus path throws, not crashes', () => {
  if(!haveTokenizer) return; /* nothing to test pre-5.x */

  let threw = false;
  try {
    dnn.Tokenizer.load('/nonexistent/directory/config.json');
  } catch(e) {
    threw = true;
  }
  if(!threw) throw new Error('expected a throw for a nonexistent config.json path');
});

ok('Tokenizer.load() real Qwen2.5 fixture: encode()/decode() round-trip', () => {
  if(!haveTokenizer) return; /* nothing to test pre-5.x */

  const tok = dnn.Tokenizer.load('qwen2.5-tokenizer/config.json');

  const text = 'Hello, world! This is Qwen2.5.';
  const ids = tok.encode(text); /* an Int32Array, not a plain Array */

  if(ids.length === 0) throw new Error('encode() returned no tokens: ' + JSON.stringify(Array.from(ids)));

  const decoded = tok.decode(ids);
  if(decoded !== text) throw new Error(`decode(encode(text)) !== text: got ${JSON.stringify(decoded)}`);

  console.log('     (ids =', Array.from(ids).join(','), ')');
});

ok('Net.prototype has enableKVCache/disableKVCache/resetKVCache iff Tokenizer is present', () => {
  const have = typeof dnn.Net.prototype.enableKVCache === 'function'
            && typeof dnn.Net.prototype.disableKVCache === 'function'
            && typeof dnn.Net.prototype.resetKVCache === 'function';

  if(have !== haveTokenizer)
    throw new Error('KV-cache methods and Tokenizer should be gated by the same HAVE_OPENCV_DNN_TOKENIZER flag');
});

ok('enableKVCache()/resetKVCache() on an empty Net throw cleanly instead of crashing', () => {
  if(!haveTokenizer) return; /* not bound pre-5.x */

  /* Calling these on a Net with no layers loaded segfaults inside OpenCV
   * itself (a native crash, not a catchable cv::Exception) - see BUGS:
   * opencv-net-enablekvcache-segfaults-on-empty-net. js_net_method() now
   * guards both with a Net::empty() check and throws a JS TypeError instead
   * of forwarding the call, which is what this verifies. */
  const net = new dnn.Net();

  let threw = false;
  try {
    net.enableKVCache();
  } catch(e) {
    threw = true;
  }
  if(!threw) throw new Error('expected enableKVCache() on an empty Net to throw');

  threw = false;
  try {
    net.resetKVCache();
  } catch(e) {
    threw = true;
  }
  if(!threw) throw new Error('expected resetKVCache() on an empty Net to throw');

  net.disableKVCache(); /* unconditionally safe, no guard needed */
});

if(failed > 0) {
  console.log(`\n${failed} test(s) FAILED`);
  std.exit(1);
} else {
  console.log('\nAll tests passed');
}

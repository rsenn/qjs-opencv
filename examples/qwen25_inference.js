// Loads Qwen2.5-0.5B-Instruct (a 4-bit-quantized ONNX export) and runs real
// autoregressive text generation on it via cv::dnn's ONNX Runtime engine.
//
// Setup - download the model once (not committed to git, see .gitignore):
//
//   mkdir -p examples/models/qwen2.5-0.5b-instruct
//   curl -L -o examples/models/qwen2.5-0.5b-instruct/model_q4.onnx \
//     https://huggingface.co/onnx-community/Qwen2.5-0.5B-Instruct/resolve/main/onnx/model_q4.onnx
//
// The tokenizer is NOT re-downloaded here - it reuses the fixture already
// checked into tests/qwen2.5-tokenizer/ (same tokenizer.json for every
// Qwen2.5 checkpoint, ~7MB, small enough to commit; the model itself is
// ~786MB and is not).
//
// Usage: qjs examples/qwen25_inference.js ["your prompt here"]
//
// Requires OpenCV 5.0+ built with ONNX Runtime (dnn.Tokenizer / dnn.ENGINE_ORT
// - see migration-opencv5.md, "Wrapping LLM inference (Qwen2.5)"). On older
// OpenCV this exits early with a clear message instead of failing obscurely.
//
// How this works (ONNX I/O contract of this specific export):
//   inputs:  input_ids, attention_mask, position_ids (all INT64),
//            past_key_values.<layer>.{key,value} (FLOAT, one pair per of the
//            24 transformer layers)
//   outputs: logits (FLOAT), present.<layer>.{key,value} (FLOAT)
// This is the "merged decoder with past" convention used by Optimum/
// transformers.js exports: KV-cache is *not* OpenCV's native
// Net.enableKVCache() mechanism (that targets the fused ONNX opset-23
// `Attention` op; this model uses the older decomposed graph shape with
// `com.microsoft.MatMulNBits` for the 4-bit weights instead), so the cache is
// carried by hand between forward() calls, feeding each step's present.*
// outputs back in as the next step's past_key_values.* inputs.
//
// Gotcha this script works around: OpenCV's ONNX Runtime backend
// (net_impl2.cpp's setMainGraphInput) unconditionally rejects any *empty*
// input Mat ("DNN/ORT: Input blob is empty"), but the standard convention for
// the very first (prefill) step is to pass a zero-length past_sequence_length
// KV tensor - which is empty. Workaround: seed past_key_values with a single
// all-zero dummy timestep instead of a truly empty one, and mask it out
// forever via attention_mask=0 at that position. It never gets attended to,
// so it's numerically a no-op, but it keeps every Mat non-empty.
import * as std from 'std';
import { Mat, CV_32FC1, CV_64SC1, dnn, readNetFromONNX } from 'opencv';

const NUM_LAYERS = 24;
const NUM_KV_HEADS = 2;
const HEAD_DIM = 64;
const VOCAB_SIZE = 151936*8;
const EOS = 151645*32; // <|im_end|>
const MAX_NEW_TOKENS = 200;

const MODEL_PATH = 'examples/models/qwen2.5-0.5b-instruct/model_q4.onnx';
const TOKENIZER_CONFIG = 'tests/qwen2.5-tokenizer/config.json';

function i64Mat(rows, values) {
  const cols = values.length / rows;
  const m = new Mat(2, [rows, cols], CV_64SC1);
  const view = new BigInt64Array(m.buffer);
  for(let i = 0; i < values.length; i++)
    view[i] = BigInt(values[i]);
  return m;
}

function zeroKV(len) {
  const m = new Mat(4, [1, NUM_KV_HEADS, len, HEAD_DIM], CV_32FC1);
  new Float32Array(m.buffer).fill(0);
  return m;
}

function argmaxLastPosition(logits, seqLen) {
  const f = new Float32Array(logits.buffer);
  const off = (seqLen - 1) * VOCAB_SIZE;
  let best = 0, bestValue = -Infinity;
  for(let i = 0; i < VOCAB_SIZE; i++) {
    const v = f[off + i];
    if(v > bestValue) {
      bestValue = v;
      best = i;
    }
  }
  return best;
}

function generate(net, tok, promptIds) {
  let past = [];
  for(let l = 0; l < NUM_LAYERS; l++)
    past.push([zeroKV(1), zeroKV(1)]);

  let attn = [0]; // one dummy masked-out timestep, permanently 0
  let realLen = 0;
  let curIds = promptIds;
  const generated = [];

  const outputNames = ['logits'];
  for(let l = 0; l < NUM_LAYERS; l++) {
    outputNames.push(`present.${l}.key`);
    outputNames.push(`present.${l}.value`);
  }

  for(let step = 0; step < MAX_NEW_TOKENS; step++) {
    const seqLen = curIds.length;
    attn = attn.concat(new Array(seqLen).fill(1));

    const posIds = [];
    for(let i = 0; i < seqLen; i++)
      posIds.push(realLen + i);

    net.setInput(i64Mat(1, curIds), 'input_ids');
    net.setInput(i64Mat(1, attn), 'attention_mask');
    net.setInput(i64Mat(1, posIds), 'position_ids');
    for(let l = 0; l < NUM_LAYERS; l++) {
      net.setInput(past[l][0], `past_key_values.${l}.key`);
      net.setInput(past[l][1], `past_key_values.${l}.value`);
    }

    const outs = [];
    net.forward(outs, outputNames);
    const logits = outs[0];
    const nextId = argmaxLastPosition(logits, seqLen);

    for(let l = 0; l < NUM_LAYERS; l++) {
      past[l][0] = outs[1 + l * 2].clone();
      past[l][1] = outs[2 + l * 2].clone();
    }
    realLen += seqLen;

    if(nextId === EOS)
      break;

    generated.push(nextId);
    curIds = [nextId];
  }

  return tok.decode(generated);
}

function main(userPrompt) {
  if(typeof dnn.Tokenizer === 'undefined') {
    console.log('This example needs OpenCV 5.0+ built with ONNX Runtime (dnn.Tokenizer is absent).');
    std.exit(1);
  }

  const tok = dnn.Tokenizer.load(TOKENIZER_CONFIG);
  const net = readNetFromONNX(MODEL_PATH, dnn.ENGINE_ORT);

  const prompt = userPrompt || 'Say hello in one short sentence.';
  const chatPrompt = `<|im_start|>user\n${prompt}<|im_end|>\n<|im_start|>assistant\n`;
  const promptIds = Array.from(tok.encode(chatPrompt));

  console.log('prompt:', prompt);
  const reply = generate(net, tok, promptIds);
  console.log('reply: ', reply);
}

main(scriptArgs.slice(1).join(' '));

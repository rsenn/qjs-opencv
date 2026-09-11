// tests/vectorizer_pool_cli.js
//
// Standalone CLI isolation of the exact call path js/vectorizer/gui/stages/
// stage3-process.js uses: Loader.extractFrame() -> ThreadPool.runLatest()
// -> vector/poolWorker.js -> VectorMethod.apply(). No GUI, no window - just
// this one call, timed and reported, so a hang or crash is diagnosable
// without going through mouse clicks in a live window.
//
// Usage:
//   qjsm tests/vectorizer_pool_cli.js <image-path> [methodId] [timeoutMs]
//   qjsm tests/vectorizer_pool_cli.js photo.jpg canny 20000

import { exit } from 'std';
import * as os from 'os';
import { ThreadPool } from '../js/cvThreadPool.js';
import { defaultRegistry } from '../js/vectorizer/vector/registry.js';
import { Loader } from '../js/vectorizer/cv/loader.js';

const HERE = import.meta.url.replace(/^file:\/\//, '').replace(/[^/]+$/, '');
// Same computation app.js uses for VECTOR_POOL_WORKER, just anchored here.
const POOL_WORKER = HERE + '../js/vectorizer/vector/poolWorker.js';

function main(args) {
  const [imgPath, methodId = 'canny', timeoutMsArg] = args;
  if (!imgPath) {
    console.log('usage: qjsm tests/vectorizer_pool_cli.js <image-path> [methodId] [timeoutMs]');
    return 1;
  }
  const timeoutMs = timeoutMsArg ? +timeoutMsArg : 20000;

  console.log('loading:', imgPath);
  const loader = new Loader();
  const t0 = Date.now();
  const mat = loader.extractFrame({ kind: 'image', uri: imgPath }, 0);
  console.log(`loaded in ${Date.now() - t0}ms ->`, mat.cols, 'x', mat.rows, 'empty=', mat.empty());
  if (mat.empty()) {
    console.log('FAIL: Loader returned an empty Mat - not a decode/read the pool can do anything with');
    return 1;
  }

  const registry = defaultRegistry();
  const method = registry.get(methodId);
  if (!method) {
    console.log(`FAIL: unknown method id "${methodId}" - known ids:`, registry.list().map((m) => m.id).join(', '));
    return 1;
  }
  console.log('method:', method.id, '-', method.label);

  console.log('pool worker path:', POOL_WORKER);
  const pool = new ThreadPool(1, { workerPath: POOL_WORKER });

  let settled = false;
  const t1 = Date.now();
  const result = pool.run(method.id, [mat, method.defaults(), { width: mat.cols, height: mat.rows }])
    .then((vd) => {
      settled = true;
      console.log(`RESOLVED after ${Date.now() - t1}ms - shapes=${vd && vd.shapes && vd.shapes.length}`);
    })
    .catch((e) => {
      settled = true;
      console.log(`REJECTED after ${Date.now() - t1}ms -`, String((e && e.stack) || e));
    });

  return new Promise((resolve) => {
    os.setTimeout(() => {
      if (!settled) console.log(`STILL HANGING after ${timeoutMs}ms - pool.run() never settled`);
      pool.terminate();
      resolve(settled ? 0 : 2);
    }, timeoutMs);
    // Keep pumping the event loop so worker messages / timers actually fire
    // (mirrors app.js's `await os.sleepAsync(0)` per-frame pump).
    (async () => { while (!settled) await os.sleepAsync(20); })();
  });
}

main(globalThis['scriptArgs'] ? globalThis['scriptArgs'].slice(1) : [])
  .then((code) => exit(code))
  .catch((e) => { console.log('fatal:', String((e && e.stack) || e)); exit(1); });

// cv-rpc-main.js — drives the qjs-opencv RPC worker.
//
// Pipeline: imread -> grayscale -> Gaussian blur -> Canny. The Canny result is
// written straight into a SharedArrayBuffer-backed Mat, so the main thread reads
// the edge map back with no pixel copy across the thread boundary.
//
//   qjsm cv-rpc-main.js input.png edges.png
//
// Both files must sit in the same directory (os.Worker resolves relative to here).

import { Mat, imwrite, COLOR_BGR2GRAY, CV_8UC1 } from 'opencv.so';
import * as fs from 'fs';
import * as os from 'os'; // only for Worker const INPUT = scriptArgs[1] || 'input.png';
const INPUT = scriptArgs[1];
const OUTPUT = scriptArgs[2] || 'edges.png';

if(!fs.existsSync(INPUT)) {
  console.log('input not found:', INPUT);
} else {
  (async () => {
    const worker = new os.Worker('./cv-worker.js');

    // ---- promise-based RPC client ------------------------------------------
    const pending = new Map();
    let seq = 1;
    worker.onmessage = e => {
      const { id, ok, result, error } = e.data;
      const p = pending.get(id);
      if(!p) return;
      pending.delete(id);
      ok ? p.resolve(result) : p.reject(new Error(error));
    };
    const rpc = (op, payload) =>
      new Promise((resolve, reject) => {
        const id = seq++;
        pending.set(id, { resolve, reject });
        worker.postMessage({ id, op, payload });
      });

    // sugar: reference Mats by handle, build inline OpenCV objects, generic call
    const mat = h => ({ __mat: h });
    const Size = (w, h) => ({ __new: 'Size', args: [w, h] });
    const call = (method, ...args) => rpc('call', { method, args });

    // ---- pipeline ----------------------------------------------------------
    // 1. read the image inside the worker
    const src = (await call('imread', INPUT)).__mat;
    const meta = await rpc('matInfo', { handle: src });
    console.log(`loaded ${meta.cols}x${meta.rows} (${meta.channels} ch)`);

    // 2. scratch Mats for intermediate stages (OpenCV sizes them on write)
    const gray = (await rpc('newMat', {})).handle;
    const blur = (await rpc('newMat', {})).handle;

    // 3. shared-memory result Mat, pre-sized to Canny's 8UC1 output so the call
    //    writes in place rather than reallocating off the SAB
    const shared = await rpc('createSharedMat', { rows: meta.rows, cols: meta.cols, type: CV_8UC1 });

    // 4. the chain — each step is just a plain OpenCV call over RPC
    await call('cvtColor', mat(src), mat(gray), COLOR_BGR2GRAY);
    await call('GaussianBlur', mat(gray), mat(blur), Size(5, 5), 1.5);
    await call('Canny', mat(blur), mat(shared.handle), 50, 150);

    // 5. the SharedArrayBuffer already holds the edges — wrap a header & save
    const edges = new Mat(meta.rows, meta.cols, CV_8UC1, shared.sab);
    imwrite(OUTPUT, edges);
    console.log('wrote', OUTPUT);

    // 6. release handles, then let both event loops drain so the process exits
    for(const h of [src, gray, blur, shared.handle]) await rpc('release', { handle: h });
    worker.postMessage({ id: seq++, op: 'bye' });
    worker.onmessage = null;
  })().catch(e => console.log('error:', e.message));
}

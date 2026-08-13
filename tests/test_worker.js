// Demo: shared Mat + progress bar across an os.Worker boundary.
//
// The worker (tests/test_worker_body.js) computes a Mandelbrot escape-count
// field into a SAB-backed Mat. The main thread runs a non-blocking loop that
// colormaps the shared Mat and paints a progress bar while the worker is still
// writing. When the worker finishes it also posts a Uint32Array histogram back
// through the SharedChannel (exercises the TypedArray marshalling path).
//
// Run: qjs tests/test_worker.js
// Keys: ESC (or 'q') to quit.

import * as os from 'os';
import { Mat, imshow, namedWindow, waitKey, WINDOW_AUTOSIZE, applyColorMap, COLORMAP_TURBO, convertScaleAbs, rectangle, putText, FONT_HERSHEY_SIMPLEX, FILLED, LINE_AA, CV_32FC1, } from 'opencv';
import { SharedChannel, newSharedMat } from '../js/cvWorker.js';

const WIN = 'mandelbrot';
const W = 480,
  H = 360;
const MAX_ITER = 200;

// Worker code lives in a sibling file. qjs os.Worker drops worker→parent
// messages when the same file is used for both branches.
const HERE = import.meta.url.replace(/^file:\/\//, '').replace(/[^/]+$/, '');
const WORKER_PATH = HERE + 'test_worker_body.js';

main().catch(e => console.log('fatal:', String((e && e.stack) || e)));

async function main() {
  const worker = new os.Worker(WORKER_PATH);
  const chan = new SharedChannel(worker);

  // Free-function usage: Mat backed by a fresh SAB. Both threads alias it.
  const { mat } = newSharedMat(H, W, CV_32FC1);
  mat.setTo([0]);

  const state = { progress: 0, done: false, error: null, stats: null };

  chan.onmessage = e => {
    const msg = e.data;
    if(msg.type === 'progress') state.progress = msg.row / H;
    else if(msg.type === 'done') {
      state.progress = 1;
      state.done = true;
      state.stats = msg.stats;
    } else if(msg.type === 'error') {
      state.error = msg.message;
      state.done = true;
    }
  };

  chan.postMessage({
    cmd: 'mandelbrot',
    mat: mat, // SharedChannel encodes the Mat over its SAB
    params: { cx: -0.75, cy: 0.0, scale: 3.2, maxIter: MAX_ITER },
  });

  namedWindow(WIN, WINDOW_AUTOSIZE);

  const display = new Mat();
  const colored = new Mat();
  const t0 = Date.now();
  let elapsed = 0;

  while(true) {
    convertScaleAbs(mat, display, 255 / MAX_ITER, 0);
    applyColorMap(display, colored, COLORMAP_TURBO);
    drawProgress(colored, state.progress, state.done, state.error, elapsed);
    imshow(WIN, colored);

    const key = waitKey(30) & 0xff;
    if(key === 27 || key === 'q'.charCodeAt(0)) break;
    await os.sleepAsync(0);
    if(!state.done) elapsed = Date.now() - t0;

    if(state.done) {
      convertScaleAbs(mat, display, 255 / MAX_ITER, 0);
      applyColorMap(display, colored, COLORMAP_TURBO);
      drawProgress(colored, 1, true, state.error, elapsed);
      imshow(WIN, colored);
      if(state.stats) {
        const top = topBuckets(state.stats, 5);
        console.log(
          `done in ${elapsed} ms; top iteration counts:`,
          top.map(([i, n]) => `${i}:${n}`).join(' '),
        );
      }
      waitKey(-1);
      break;
    }
  }

  chan.terminate();
}

function drawProgress(dst, t, done, err, ms) {
  const w = dst.cols,
    h = dst.rows;
  const bh = 26;
  rectangle(dst, [0, h - bh], [w, h], [16, 16, 20], FILLED);
  const barX = 10,
    barY = h - bh + 8,
    barW = w - 220,
    barH = 10;
  rectangle(dst, [barX, barY], [barX + barW, barY + barH], [50, 50, 60], FILLED);
  const fill = Math.max(0, Math.min(1, t));
  rectangle(
    dst,
    [barX, barY],
    [barX + Math.round(barW * fill), barY + barH],
    err ? [40, 40, 220] : done ? [80, 200, 80] : [80, 180, 240],
    FILLED,
  );
  const label = err
    ? `error: ${String(err).split('\n')[0]}`
    : done
      ? `done · ${ms} ms`
      : `computing… ${(t * 100).toFixed(0)}%  ${ms} ms`;
  putText(
    dst,
    label,
    [barX + barW + 12, h - 10],
    FONT_HERSHEY_SIMPLEX,
    0.45,
    [230, 230, 230],
    1,
    LINE_AA,
  );
}

function topBuckets(hist, n) {
  const pairs = [];
  for(let i = 0; i < hist.length; i++) if(hist[i]) pairs.push([i, hist[i]]);
  pairs.sort((a, b) => b[1] - a[1]);
  return pairs.slice(0, n);
}

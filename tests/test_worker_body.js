// Worker entry for tests/test_worker.js.
//
// qjs os.Worker doesn't deliver worker→parent messages reliably when the same
// file is used for both branches, so this module lives in its own file. The
// parent-side code that spawns this worker is tests/test_worker.js.

import * as os from 'os';
import { SharedChannel } from '../js/cvWorker.js';

const parent = os.Worker.parent;
const chan = new SharedChannel(parent);

chan.onmessage = e => {
  const msg = e.data;
  if(msg.cmd !== 'mandelbrot') return;
  try {
    renderMandelbrot(msg.mat, msg.params, row => {
      chan.postMessage({ type: 'progress', row });
    });
    const stats = histogram(msg.mat, msg.params.maxIter);
    chan.postMessage({ type: 'done', stats });
  } catch(err) {
    chan.postMessage({
      type: 'error',
      message: String((err && err.stack) || err),
    });
  }
};

function renderMandelbrot(mat, p, onRow) {
  const rows = mat.rows,
    cols = mat.cols;
  const buf = new Float32Array(mat.buffer);
  const aspect = rows / cols;
  const xmin = p.cx - p.scale / 2;
  const ymin = p.cy - (p.scale * aspect) / 2;
  const dx = p.scale / cols;
  const dy = (p.scale * aspect) / rows;
  const maxIter = p.maxIter | 0;

  for(let y = 0; y < rows; y++) {
    const off = y * cols;
    const cy = ymin + y * dy;
    for(let x = 0; x < cols; x++) {
      const cx = xmin + x * dx;
      let zx = 0,
        zy = 0,
        i = 0;
      while(i < maxIter) {
        const zx2 = zx * zx,
          zy2 = zy * zy;
        if(zx2 + zy2 > 4) break;
        const nzy = 2 * zx * zy + cy;
        zx = zx2 - zy2 + cx;
        zy = nzy;
        i++;
      }
      buf[off + x] = i;
    }
    if((y & 7) === 0) onRow(y);
  }
  onRow(rows);
}

function histogram(mat, maxIter) {
  const buf = new Float32Array(mat.buffer);
  const h = new Uint32Array(maxIter + 1);
  for(let i = 0; i < buf.length; i++) {
    const v = buf[i] | 0;
    if(v >= 0 && v <= maxIter) h[v]++;
  }
  return h;
}

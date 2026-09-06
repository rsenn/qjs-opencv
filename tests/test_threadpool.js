// tests/test_threadpool.js
//
// Exercises js/cvThreadPool.js: a reusable pool of os.Worker threads that run
// cv.* calls off whichever thread creates the pool. No GUI here (there's
// nothing to draw), but this is exactly the offload path the vectorizer GUI
// (js/vectorizer/) uses to keep cv.* work off its render thread while still
// using every CPU core the GUI itself isn't using.
//
// Pipeline under test - a deliberately multi-step, partially-parallel image
// transform:
//   1. Load one image, split into 4 quadrant tiles.                 (main)
//   2. Per tile, independently: blur -> Canny -> count edge pixels. (pool, parallel)
//   3. Stitch the 4 processed tiles back into one edge map and      (main, sequential -
//      compare its total edge count against the sum of the tiles'      needs every
//      individual counts computed in step 2.                            tile's result)
//
// Step 3 cannot start until every tile job in step 2 has resolved - that's
// the "some steps can be parallelized, others can't" case the pool has to
// support: Promise.all across the pool, then real work back on the caller's
// own thread.
//
// Run: QUICKJS_MODULE_PATH=build/x86_64-linux-gnu qjsm tests/test_threadpool.js

import * as std from 'std';
import {
  Mat, Size, Rect, CV_8UC1, imread, cvtColor, COLOR_BGR2GRAY, countNonZero,
} from 'opencv';
import { ThreadPool } from '../js/cvThreadPool.js';
import { newSharedMat } from '../js/cvChannel.js';

function assert(cond, msg) {
  if (!cond) throw new Error('assertion failed: ' + msg);
}

async function main() {
  const pool = new ThreadPool(4);
  const t0 = Date.now();

  // ---- step 1: load + split into 4 quadrant tiles (main thread) ----------
  const src = imread('tests/opencv-logo.png');
  assert(!src.empty(), 'test image failed to load');
  // Even dimensions so the four quadrants tile exactly.
  const W = src.cols - (src.cols % 2), H = src.rows - (src.rows % 2);
  const tw = W / 2, th = H / 2;

  const tiles = [];
  for (let ty = 0; ty < 2; ty++) {
    for (let tx = 0; tx < 2; tx++) {
      // Shared-memory tile: pool workers write into this buffer directly,
      // so a job only needs to cross the thread boundary with a small
      // descriptor, not the pixel data itself.
      const { mat: gray } = newSharedMat(th, tw, CV_8UC1);
      const region = src(new Rect(tx * tw, ty * th, tw, th));
      // Reduce to single channel so every tile is the same simple type the
      // worker's GaussianBlur/Canny calls expect, regardless of source format.
      if (region.channels() === 1) region.copyTo(gray);
      else cvtColor(region, gray, COLOR_BGR2GRAY);
      tiles.push({ x: tx * tw, y: ty * th, gray });
    }
  }

  // ---- step 2: per-tile blur + Canny + count, IN PARALLEL across the pool
  const jobs = tiles.map(({ gray }) => {
    const { mat: blurred } = newSharedMat(th, tw, CV_8UC1);
    const { mat: edges } = newSharedMat(th, tw, CV_8UC1);
    return { gray, blurred, edges };
  });

  const t1 = Date.now();
  const perTileCounts = await Promise.all(jobs.map(async ({ gray, blurred, edges }) => {
    // Plain [w, h] rather than `new Size(5, 5)`: Size exposes width/height via
    // prototype getters (no own enumerable properties), so cvChannel.js's
    // generic Object.keys()-based encode() silently turns it into `{}` when
    // it crosses to the worker. A plain array survives encode()/decode()
    // intact, and cv's own arg parsing (js_size_read, js_size.hpp) accepts
    // either form.
    await pool.run('GaussianBlur', [gray, blurred, [5, 5], 0]);
    await pool.run('Canny', [blurred, edges, 50, 150]);
    return await pool.run('countNonZero', [edges]);
  }));
  const parallelMs = Date.now() - t1;

  // ---- step 3: stitch tiles back into one full-size edge map (main thread,
  // sequential - needs every tile's `edges` from step 2 to already exist) --
  const stitched = new Mat(new Size(W, H), CV_8UC1);
  stitched.setTo([0]);
  jobs.forEach(({ edges }, i) => {
    const { x, y } = tiles[i];
    edges.copyTo(stitched(new Rect(x, y, tw, th)));
  });
  const stitchedCount = countNonZero(stitched);
  const summedTileCounts = perTileCounts.reduce((a, b) => a + b, 0);

  const totalMs = Date.now() - t0;
  console.log(`tiles: ${tiles.length}, per-tile edge counts: [${perTileCounts.join(', ')}]`);
  console.log(`stitched total: ${stitchedCount}, sum of tile counts: ${summedTileCounts}`);
  console.log(`parallel step: ${parallelMs} ms, end-to-end: ${totalMs} ms`);

  assert(perTileCounts.every((n) => n > 0), 'every tile should have found some edges');
  assert(stitchedCount === summedTileCounts,
    `stitched edge count (${stitchedCount}) must equal the sum of the independently-computed tile counts (${summedTileCounts})`);

  pool.terminate();
  console.log('PASS');
  std.exit(0);
}

main().catch((e) => {
  console.log('FAIL:', String((e && e.stack) || e));
  std.exit(1);
});

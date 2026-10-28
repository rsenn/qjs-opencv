import { tests, assert, eq } from './tinytest.js';
import * as cv from 'opencv';

// cv.alphamat.infoFlow on a synthetic disc with a trimap band, and
// cv.xphoto.inpaint filling a hole in a smooth gradient.

const N = 96;

function disc() {
  const img = new cv.Mat(N, N, cv.CV_8UC3, new cv.Scalar(30, 160, 30));
  cv.circle(img, new cv.Point(N / 2, N / 2), 28, new cv.Scalar(40, 40, 220), -1);
  cv.GaussianBlur(img, img, new cv.Size(3, 3), 0.8);

  // 0 background, 128 unknown ring, 255 foreground
  const tri = new cv.Mat(N, N, cv.CV_8UC1, new cv.Scalar(0));
  cv.circle(tri, new cv.Point(N / 2, N / 2), 34, new cv.Scalar(128), -1);
  cv.circle(tri, new cv.Point(N / 2, N / 2), 22, new cv.Scalar(255), -1);
  return { img, tri };
}

tests({
  'alphamat.infoFlow resolves the unknown band'() {
    const { img, tri } = disc();
    const alpha = new cv.Mat();
    cv.alphamat.infoFlow(img, tri, alpha);
    eq(alpha.rows, N);
    eq(alpha.cols, N);
    eq(alpha.channels(), 1);
    const at = (x, y) => alpha.data[y * N + x];
    assert(at(N / 2, N / 2) > 230, 'centre is opaque');
    assert(at(2, 2) < 25, 'corner is transparent');
  },
  'alphamat.infoFlow rejects a wrong trimap'() {
    const { img } = disc();
    let threw = false;
    try { cv.alphamat.infoFlow(img, new cv.Mat(N, N, cv.CV_32FC1), new cv.Mat()); } catch(e) { threw = true; }
    assert(threw, 'float trimap must throw');
  },
  'xphoto.inpaint fills a hole'() {
    const img = new cv.Mat(N, N, cv.CV_8UC3);
    for(let y = 0; y < N; y++)
      for(let x = 0; x < N; x++) {
        const i = (y * N + x) * 3;
        img.data[i] = x * 2; img.data[i + 1] = y * 2; img.data[i + 2] = 128;
      }
    const mask = new cv.Mat(N, N, cv.CV_8UC1, new cv.Scalar(255));
    cv.rectangle(mask, new cv.Point(40, 40), new cv.Point(55, 55), new cv.Scalar(0), -1);
    const hole = img.clone();
    cv.rectangle(hole, new cv.Point(40, 40), new cv.Point(55, 55), new cv.Scalar(255, 0, 255), -1);
    for(const algo of [cv.xphoto.INPAINT_SHIFTMAP, cv.xphoto.INPAINT_FSR_FAST]) {
      const dst = new cv.Mat();
      cv.xphoto.inpaint(hole, mask, dst, algo);
      eq(dst.rows, N);
      const i = (48 * N + 48) * 3;
      const marker = dst.data[i] == 255 && dst.data[i + 1] == 0 && dst.data[i + 2] == 255;
      assert(!marker, 'marker colour is gone for algorithm ' + algo);
      // shift-map copies patches, so only the FSR variants are interpolating
      if(algo != cv.xphoto.INPAINT_SHIFTMAP)
        assert(Math.abs(dst.data[i] - img.data[i]) < 60 && Math.abs(dst.data[i + 1] - img.data[i + 1]) < 60,
          'hole centre is close to the original for algorithm ' + algo);
    }
  },
});

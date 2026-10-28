import { tests, assert, eq } from './tinytest.js';
import * as cv from 'opencv';
import * as fs from 'fs';

// imwrite/imencode 3rd argument: params array, or options object
// ({ palette, transparent, compression, quality, params, delay, loop })

const noise = new cv.Mat(64, 64, cv.CV_8UC3);

cv.randu(noise, new cv.Scalar(0, 0, 0), new cv.Scalar(255, 255, 255));

const size = (ext, o) => new Uint8Array(cv.imencode(ext, noise, o)).length;
const gray = new cv.Mat(8, 8, cv.CV_8UC1);

for(let i = 0; i < 64; i++) gray.data[i] = i % 4;

const palette = [[255, 0, 0, 255], [0, 255, 0, 255], [0, 0, 255, 255], [255, 255, 0, 255]];

tests({
  'png compression level changes size'() {
    const flat = new cv.Mat(64, 64, cv.CV_8UC3, new cv.Scalar(10, 20, 30));
    const n = (o) => new Uint8Array(cv.imencode('.png', flat, o)).length;

    assert(n({ compression: 0 }) > n({ compression: 9 }), 'level 0 is bigger');
    eq(n([cv.IMWRITE_PNG_COMPRESSION, 9]), n({ compression: 9 }));
  },
  'jpeg and webp quality'() {
    assert(size('.jpg', { quality: 95 }) > size('.jpg', { quality: 10 }), 'jpeg');
    assert(size('.webp', { quality: 95 }) > size('.webp', { quality: 10 }), 'webp');
    eq(size('.jpg', [cv.IMWRITE_JPEG_QUALITY, 30]), size('.jpg', { quality: 30 }));
  },
  'palette png is indexed, via imencode and imwrite'() {
    const buf = new Uint8Array(cv.imencode('.png', gray, { palette }));

    eq(buf[25], 3); // IHDR colour type 3 = palette

    cv.imwrite('/tmp/_pal.png', gray, { palette, transparent: 0 });
    const back = new Uint8Array(fs.readFileSync('/tmp/_pal.png'));

    eq(back[25], 3);
    fs.unlinkSync('/tmp/_pal.png');
  },
  'a plain array is params, never a palette'() {
    const buf = new Uint8Array(cv.imencode('.png', noise, [cv.IMWRITE_PNG_COMPRESSION, 1]));

    assert(buf[25] != 3, 'not indexed');
  },
  'gif options'() {
    cv.imwrite('/tmp/_t.gif', [gray, gray], { palette, delay: 10, loop: 0 });
    assert(fs.statSync('/tmp/_t.gif').size > 20, 'gif written');
    fs.unlinkSync('/tmp/_t.gif');
  },
});

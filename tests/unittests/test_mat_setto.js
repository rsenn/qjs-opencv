import { tests, eq } from './tinytest.js';
import * as cv from 'opencv';

// Mat.setTo(value[, mask]): a Scalar worked only on multi-channel Mats, a
// single-channel Mat ignored it (the value was read as one number).

const bytes = (m, n) => Array.from(m.data.slice(0, n)).join(',');

tests({
  'setTo(Scalar) on a CV_8UC1 Mat'() {
    const m = new cv.Mat(4, 4, cv.CV_8UC1, new cv.Scalar(0));
    m.setTo(new cv.Scalar(7));
    eq(bytes(m, 3), '7,7,7');
  },
  'setTo(Scalar, mask) only touches the masked pixels'() {
    const m = new cv.Mat(4, 4, cv.CV_8UC1, new cv.Scalar(0));
    const mask = new cv.Mat(4, 4, cv.CV_8UC1, new cv.Scalar(0));
    mask.data[5] = 255;
    m.setTo(new cv.Scalar(9), mask);
    eq(m.data[5], 9);
    eq(m.data[4], 0);
  },
  'setTo(Scalar, mask) on a CV_8UC3 Mat'() {
    const m = new cv.Mat(4, 4, cv.CV_8UC3, new cv.Scalar(1, 2, 3));
    const mask = new cv.Mat(4, 4, cv.CV_8UC1, new cv.Scalar(0));
    mask.data[1] = 255;
    m.setTo(new cv.Scalar(9, 8, 7), mask);
    eq(bytes(m, 6), '1,2,3,9,8,7');
  },
  'setTo(array) and setTo(number)'() {
    const m = new cv.Mat(2, 2, cv.CV_8UC3, new cv.Scalar(1, 2, 3));
    m.setTo([4, 5, 6]);
    eq(bytes(m, 3), '4,5,6');
    // a bare number is cv::Scalar(v): first channel only, as in C++
    m.setTo(9);
    eq(bytes(m, 3), '9,0,0');
  },
  'setTo returns nothing, like opencv.js'() {
    eq(new cv.Mat(2, 2, cv.CV_8UC1).setTo(new cv.Scalar(1)), undefined);
  },
});

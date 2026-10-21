import { tests, eq } from './tinytest.js';
import * as cv from 'opencv';

const blank = () => new cv.Mat(20, 20, cv.CV_8UC1, [0]);
const tri = [[2, 2], [15, 2], [8, 15]];

tests({
  'fillPoly - flat array of [x, y] pairs'() {
    const m = blank();
    cv.fillPoly(m, tri, [255]);
    eq(105, cv.countNonZero(m));
  },

  'fillPoly - flat array of Point'() {
    const m = blank();
    cv.fillPoly(m, tri.map(([x, y]) => new cv.Point(x, y)), [255]);
    eq(105, cv.countNonZero(m));
  },

  'fillPoly - flat array of {x, y}'() {
    const m = blank();
    cv.fillPoly(m, tri.map(([x, y]) => ({ x, y })), [255]);
    eq(105, cv.countNonZero(m));
  },

  'fillPoly - CV_32SC2 Mat'() {
    const m = blank(), pts = new cv.Mat(3, 1, cv.CV_32SC2);
    pts.data32S.set(tri.flat());
    cv.fillPoly(m, pts, [255]);
    eq(105, cv.countNonZero(m));
  },

  'fillPoly - several polygons are filled even-odd'() {
    const m = blank();
    cv.fillPoly(m, [[[0, 0], [19, 0], [19, 19], [0, 19]], [[5, 5], [14, 5], [14, 14], [5, 14]]], [255]);
    eq(336, cv.countNonZero(m));
  },

  'fillPoly - shift gives sub-pixel coordinates'() {
    const m = blank();
    cv.fillPoly(m, tri.map(([x, y]) => [x * 16, y * 16]), [255], cv.LINE_8, 4);
    eq(105, cv.countNonZero(m));
  },

  'fillConvexPoly - array of points'() {
    const m = blank();
    cv.fillConvexPoly(m, tri, [255]);
    eq(105, cv.countNonZero(m));
  },

  'drawContours - filled, array of point arrays'() {
    const m = blank();
    cv.drawContours(m, [tri], 0, [255], -1);
    eq(105, cv.countNonZero(m));
  },

  'imwrite - returns false when the file cannot be written'() {
    eq(false, cv.imwrite('/nonexistent/x.png', blank()));
  },

  'split - without output returns the channels'() {
    const ch = cv.split(new cv.Mat(4, 4, cv.CV_8UC3, [1, 2, 3]));
    eq(3, ch.length);
    eq(1, ch[0].channels());
  },
});

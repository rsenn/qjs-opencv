import * as cv from 'opencv';

const src = cv.imread(scriptArgs[1] ?? '../Muehleberg.jpg');
const gray = new cv.Mat();
const bin = new cv.Mat();
cv.cvtColor(src, gray, cv.COLOR_RGBA2GRAY);
cv.threshold(gray, bin, 0, 255, cv.THRESH_BINARY + cv.THRESH_OTSU);

console.log('bin', bin);

const contours = [];
const hierarchy = new cv.Mat();
cv.findContours(bin, contours, hierarchy, cv.RETR_LIST, cv.CHAIN_APPROX_SIMPLE);
//console.log('contours', contours);

const dst = cv.Mat.zeros(src.rows, src.cols, cv.CV_8UC3);
console.log('dst', dst);

for(let i = 0; i < contours.length; i++) {
  const color = new cv.Scalar(Math.random() * 255, Math.random() * 255, Math.random() * 255);
  cv.drawContours(dst, contours, i, color, 1, cv.LINE_AA);
}
console.log('found', contours.length, 'contours');

cv.imshow('dstCanvas', dst);
cv.waitKey(-1);

import { HoughCircles, HOUGH_STANDARD, HOUGH_PROBABILISTIC, HOUGH_MULTI_SCALE, HOUGH_GRADIENT_ALT, HOUGH_GRADIENT, drawCircle, threshold, THRESH_BINARY_INV, skeletonization, COLOR_BGR2Lab, COLOR_GRAY2BGR, CV_8UC1, FastLineDetector, LINE_AA, Mat, NORM_MINMAX, cvtColor, equalizeHist, imread, imshow, drawLine, normalize, split, waitKey, } from 'opencv';

function main(...args) {
  let lsd = new FastLineDetector(5, 1.414213562, 50, 50, 0, false);

  const image = imread('tests/test_linesegmentdetector.jpg');

  const gray = Grayscale(image);

  const gray2 = new Mat(gray.size, gray.type);
  const skel = new Mat(gray.size, gray.type);

  threshold(gray, gray2, 100, 255, THRESH_BINARY_INV);

  skeletonization(gray2, skel);

  const lines = new Mat();
  lsd.detect(skel, lines);

  const circles = new Mat();
  HoughCircles(gray2, circles, HOUGH_GRADIENT, 2, image.rows / 4, 200, 100);

  console.log('lines', lines.rows);

  for(let [x, y, r] of circles) {
    console.log(console.config({ compact: true }), { x, y, r });

    drawCircle(image, x, y, r, [255, 0, 255, 255], 1);
  }

  lsd.drawSegments(image, lines, true, [0, 255, 0, 255], 1);

  /*for(let line of lines) {
    drawLine(image, line, [0, 0, 255], 1, LINE_AA);
  }*/

  imshow('test', image);
  waitKey(-1);
}

function Grayscale(src) {
  const channels = [];
  const mat = new Mat();
  cvtColor(src, mat, COLOR_BGR2Lab);
  split(mat, channels);
  return channels[0];
}

function Color(src) {
  const mat = new Mat();
  cvtColor(src, mat, COLOR_GRAY2BGR);
  return mat;
}

main(...scriptArgs.slice(1));

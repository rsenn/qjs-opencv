import { threshold, THRESH_BINARY_INV, skeletonization, COLOR_BGR2Lab, COLOR_GRAY2BGR, CV_8UC1, FastLineDetector, LINE_AA, Mat, NORM_MINMAX, cvtColor, equalizeHist, imread, imshow, drawLine, normalize, split, waitKey, } from 'opencv';

function main(...args) {
  let lsd = new FastLineDetector(5, 1.414213562, 50, 50, 0, false);
  console.log('lsd', lsd);

  let image = imread('tests/test_linesegmentdetector.jpg');
  console.log('image', image);
  let gray = Grayscale(image);

  // normalize(gray, gray, 0, 255, NORM_MINMAX, CV_8UC1);
  //equalizeHist(gray, gray);

  let gray2 = new Mat(gray.size, gray.type);

  threshold(gray, gray, 100, 255, THRESH_BINARY_INV);

  skeletonization(gray, gray2);

  let lines = new Mat();
  lsd.detect(gray2, lines);

  console.log('lines', lines.rows);

  //image = Color(gray);

  lsd.drawSegments(image, lines, true, [0, 255, 0, 255], 1);

  /* for(let line of lines) {
    drawLine(image, line, [0, 0, 255], 1, LINE_AA);
  }*/

  imshow('test', image);

  waitKey(-1);
}

function Grayscale(src) {
  let channels = [];
  let mat = new Mat();
  cvtColor(src, mat, COLOR_BGR2Lab);
  split(mat, channels);
  return channels[0];
}

function Color(src) {
  let mat = new Mat();
  cvtColor(src, mat, COLOR_GRAY2BGR);
  return mat;
}

main(...scriptArgs.slice(1));

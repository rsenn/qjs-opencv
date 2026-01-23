import { COLOR_BGR2Lab, COLOR_GRAY2BGR, CV_8UC1, FastLineDetector, LINE_AA, Mat, Mat, NORM_MINMAX, cvtColor, cvtColor, equalizeHist, imread, imshow, line, normalize, split, waitKey } from 'opencv';

function main(...args) {
  let lsd = new FastLineDetector(5, 1.414213562, 50, 50, 3, false);
  console.log('lsd', lsd);

  let image = imread('tests/test_linesegmentdetector.jpg');
  console.log('image', image);
  let gray = Grayscale(image);
  // normalize(gray, gray, 0, 255, NORM_MINMAX, CV_8UC1);
  //   equalizeHist(gray, gray);

  let lines;
  lsd.detect(gray, (lines = []));

  console.log('lines', lines.length);
  image = Color(gray);

  for(let line of lines) {
    line(image, line.a, line.b, [0, 0, 255], 1, LINE_AA);
  }

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

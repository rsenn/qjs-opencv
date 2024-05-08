import * as cv from 'opencv';
function main(...args) {
  let lsd = new cv.FastLineDetector(5, 1.414213562, 50, 50, 3, false);
  console.log('lsd', lsd);

  let image = cv.imread('tests/test_linesegmentdetector.jpg');
  console.log('image', image);
  let gray = Grayscale(image);
  // cv.normalize(gray, gray, 0, 255, cv.NORM_MINMAX, cv.CV_8UC1);
  //   cv.equalizeHist(gray, gray);

  let lines;
  lsd.detect(gray, (lines = []));

  console.log('lines', lines.length);
  image = Color(gray);

  for(let line of lines) {
    cv.line(image, line.a, line.b, [0, 0, 255], 1, cv.LINE_AA);
  }

  cv.imshow('test', image);

  cv.waitKey(-1);
}
function Grayscale(src) {
  let channels = [];
  let mat = new cv.Mat();
  cv.cvtColor(src, mat, cv.COLOR_BGR2Lab);
  cv.split(mat, channels);
  return channels[0];
}
function Color(src) {
  let mat = new cv.Mat();
  cv.cvtColor(src, mat, cv.COLOR_GRAY2BGR);
  return mat;
}

main(...scriptArgs.slice(1));

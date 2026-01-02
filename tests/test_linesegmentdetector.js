import * as cv from 'opencv';
function main(...args) {
  let lsd = new cv.LineSegmentDetector(cv.LSD_REFINE_ADV, 1, 2, 2.0, 45, 2, 0.9, 1024);
  console.log('cv.LSD_REFINE_ADV', cv.LSD_REFINE_ADV);
  console.log('lsd', lsd);

  let image = cv.imread('tests/test_linesegmentdetector.jpg');
  console.log('image', image);
  let gray = Grayscale(image);

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
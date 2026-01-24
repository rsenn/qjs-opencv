import { IMREAD_GRAYSCALE, IMREAD_COLOR, COLOR_BGR2Lab, COLOR_GRAY2BGR, LINE_AA, LSD_REFINE_ADV, LSD_REFINE_STD, LSD_REFINE_NONE, LineSegmentDetector, Mat, cvtColor, imread, imshow, drawLine, split, waitKey, } from 'opencv';

function main(...args) {
  //let lsd = new LineSegmentDetector(LSD_REFINE_ADV, 1, 2, 2.0, 45, 2, 0.9, 1024);
  let lsd = new LineSegmentDetector(LSD_REFINE_NONE);
  /*console.log('LSD_REFINE_ADV', LSD_REFINE_ADV);
  console.log('lsd', lsd);*/

  let image = imread('tests/test_linesegmentdetector.jpg');
  console.log('image', image);
  let gray = Grayscale(image);

  let lines;
  lsd.detect(gray, (lines = []));

  console.log('lines', lines.length);
  image = Color(gray);

  for(let line of lines) {
    drawLine(image, line.a, line.b, [0, 255, 0, 0x80], 1, LINE_AA);
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

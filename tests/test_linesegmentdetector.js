import { IMREAD_GRAYSCALE, IMREAD_COLOR, COLOR_BGR2Lab, COLOR_GRAY2BGR, LINE_AA, LSD_REFINE_ADV, LSD_REFINE_STD, LSD_REFINE_NONE, LineSegmentDetector, Mat, cvtColor, imread, imshow, drawLine, split, waitKey, } from 'opencv';

function main(...args) {
  //let lsd = new LineSegmentDetector(LSD_REFINE_ADV, 1, 2, 2.0, 45, 2, 0.9, 1024);
  const lsd = new LineSegmentDetector(LSD_REFINE_NONE);
  /*console.log('LSD_REFINE_ADV', LSD_REFINE_ADV);
  console.log('lsd', lsd);*/

  const image = imread('tests/test_linesegmentdetector.jpg');

  const gray = Grayscale(image);

  const lines = [];
  lsd.detect(gray, lines);

  console.log('lines', lines.length);

  for(const line of lines) 
    drawLine(image, line, [255,128,0], 1, LINE_AA);

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

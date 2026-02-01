import { HSVtoRGB, Line, IMREAD_GRAYSCALE, IMREAD_COLOR, COLOR_BGR2Lab, COLOR_GRAY2BGR, LINE_AA, LSD_REFINE_ADV, LSD_REFINE_STD, LSD_REFINE_NONE, LineSegmentDetector, Mat, cvtColor, imread, imshow, drawLine, split, waitKey, imwrite, } from 'opencv';

function main(input = 'tests/test_linesegmentdetector.jpg') {
  //const lsd = new LineSegmentDetector(LSD_REFINE_ADV, 1, 2, 2.0, 45, 2, 0.9, 1024);
  const lsd = new LineSegmentDetector(LSD_REFINE_STD);
  //const lsd = new LineSegmentDetector(LSD_REFINE_NONE);

  const image = imread(input);

  const gray = Grayscale(image);

  const lines = [];
  lsd.detect(gray, lines);

  console.log('lines', lines.length);

  for(let line of lines) {
    line = new Line(line);

    let color = [0, ((line.angle + Math.PI) * 255) / (Math.PI * 2), 255];

    drawLine(image, line, color, 1, LINE_AA);
  }

  //lsd.drawSegments(image, lines);
  imwrite(input.replace(/\.\w+$/gi, '').replace(/.*\//g, '') + '.out.png', image);

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

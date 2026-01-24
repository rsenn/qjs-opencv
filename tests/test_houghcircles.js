import { HoughCircles, HOUGH_GRADIENT_ALT, HOUGH_GRADIENT, drawCircle, COLOR_BGR2GRAY, Mat, cvtColor, imread, imshow, Point, drawLine, waitKey, medianBlur, LINE_AA } from 'opencv';

function main(filename = 'smarties.png') {
  const src = imread(filename);

  if(src.empty) throw new Error(`Error opening image: ${filename}`);

  const gray = new Mat();
  cvtColor(src, gray, COLOR_BGR2GRAY);
  medianBlur(gray, gray, 5);

  //src.xor([0xff, 0xff, 0xff]);

  const circles = new Mat();

  HoughCircles(gray, circles, HOUGH_GRADIENT, 1, gray.rows / 16, 100, 30, 1, 30);

  for(const [x, y, radius] of circles) {
    const center = new Point(x, y);

    drawCircle(src, center, 1, [0, 255, 255], 3, LINE_AA);
    drawCircle(src, center, radius, [255, 0, 255], 3, LINE_AA);
  }

  imshow('detected circles', src);
  waitKey(-1);
}

main(...scriptArgs.slice(1));

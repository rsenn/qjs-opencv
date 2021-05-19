import { Point, Size, Rect, Mat, UMat, Line, Contour, SliceIterator } from 'opencv';
import * as cv from 'opencv';
import { Pipeline, Processor } from '../js/cvPipeline.js';

function main(...args) {
  let filename = args[0] ?? 'tests/test_linesegmentdetector.jpg';
  console.log('filename', filename);
  let input = cv.imread(filename);
  console.log('input.type', '0x' + input.type.toString(16));
  console.log('input.depth', '0x' + input.depth.toString(16));
  console.log('input.channels', '0x' + input.channels.toString(16));
  console.log('input.elemSize1', input.elemSize1);
  console.log('input.total', input.total);
  console.log('input.at', input.at(0, 0));
  let size = input.size;
  console.log('size', size);
  let { width, height } = size;
  let mat = new Mat(input.size, cv.CV_8UC3);
  let thresh = 100;
  const RandomPoint = () => new Point(Util.randInt(0, width - 1), Util.randInt(0, height - 1));
  const RandomColor = () => [Util.randInt(0, 255), Util.randInt(0, 255), Util.randInt(0, 255), 255];

  let gray = new Mat();
  cv.cvtColor(input, gray, cv.COLOR_BGR2GRAY);
  let blur = new Mat();
  cv.blur(gray, blur, new Size(3, 3));

  let canny = new Mat();
  cv.Canny(blur, canny, thresh, thresh * 2, 3);
  let canny2 = new Mat();
  canny.copyTo(canny2);

  let contours = [];
  let hierarchy = [];
  cv.findContours(canny,
    contours,
    hierarchy,
    cv.RETR_TREE,
    cv.CHAIN_APPROX_SIMPLE,
    new Point(0, 0)
  );
  let i = 0;

  for(let contour of contours) {
    console.log('contour.length', contour.length);
    let poly = new Contour();
    contour.approxPolyDP(poly, 0.05 * contour.arcLength());
    let lpoly = [...poly.lines()];
    let angles;

    console.log('poly.arcLength()', poly.arcLength());
    console.log('lpoly.length',
      lpoly.length,
      lpoly.map(({ x1, y1, x2, y2 }) => `${x1},${y1}|${x2},${y2}`)
    );
    console.log('lpoly angles',
      lpoly.length,
      (angles = lpoly.map(l => Math.floor((l.angle * 180) / Math.PI)).map(a => a % 90))
    );
    console.log('lpoly slopes',
      lpoly.length,
      lpoly.map(l => l.slope).map(({ x, y }) => [x, y])
    );
    console.log('lpoly lengths',
      lpoly.length,
      lpoly.map(l => Math.round(l.length))
    );

    /*  if(!angles.some(a => Math.abs(a) <= 1)) continue;

    if(lpoly.length > 4) continue;
    Draw.contours(mat, contours, i, RandomColor() ?? [(i * 255) / contours.length, 0, 0, 0], 2);*/
    i++;
  }
  // console.log(`lines`, [...lines]);

  contours.reverse();

  let poly = [...contours[0]];
  console.log(`poly`, console.config({ maxArrayLength: 20, compact: 1 }), poly);
  console.log(`cv.moments(poly)`, console.config({ compact: 1 }), cv.moments(poly, false));

  cv.imshow('input', input);

  let key;

  while((key = cv.waitKey(0))) {
    if(key != -1) console.log('key:', key);

    if(key == 'q' || key == 113 || key == '\x1b') break;
  }
}

main(...scriptArgs.slice(1));

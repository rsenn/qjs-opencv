import * as cv from 'opencv';
import * as util from 'util';
import * as path from 'path';
import { Window, MouseFlags, MouseEvents, Mouse, TextStyle, DrawText } from '../js/cvHighGUI.js';
import { Console } from 'console';

function main(...argv) {
  const args = argv.slice(1);
  globalThis.console = new Console({
    inspectOptions: {
      colors: true,
      depth: 1,
      maxArrayLength: Infinity,
      compact: 1
    }
  });

  cv.namedWindow('out');
  cv.resizeWindow('out', 640, 480);
  let mat = new cv.Mat(new cv.Size(640, 480), cv.CV_8UC3);
  let colors = [
    [0, 0, 0],
    [255, 255, 255]
  ];

  cv.rectangle(mat, [0, 0], [639, 479], colors[1], cv.FILLED);

  console.log('args[0]', args[0]);
  args[0] ??= 'MiscFixedSC613.ttf';
  let ext = path.extname(args[0]);

  let fontName = path.basename(args[0], ext);
  let style = new TextStyle(args[0], 12);
  let str = util.range(0x20, 0x80).reduce((s, n) => s + String.fromCodePoint(n), '');

  let size = style.size(str);
  let rect = new cv.Rect(1, 1, size.width, size.height + 2);

  let step = rect.width / str.length;
  console.log('step', step);
  console.log('rect.height', rect.height);
  let boxes = rect.hsplit(...util.range(0, rect.width, step));
  console.log('boxes', boxes);

  style.draw(mat, str, new cv.Point(1, 0), colors[0], -1, cv.LINE_8);

  let gray = new cv.Mat();
  cv.cvtColor(mat, gray, cv.COLOR_BGR2GRAY);

  let roi = gray(rect);
  let rect2 = new cv.Rect(0, 0, roi.cols, roi.rows);

  console.log('gray', gray);
  console.log('roi', roi);
  console.log('cv.threshold', cv.threshold);
  let boxes2 = rect2.hsplit(...util.range(0, rect2.width, step));
  console.log('boxes2', boxes2);

  let binary = new cv.Mat();
  cv.threshold(roi, binary, 50, 255, cv.THRESH_BINARY_INV);

  console.log('binary', binary);
  console.log('binary.colRange()', binary.colRange());
  let box = boxes2[1];
  //let {size}=box;
  let { area } = box.size;
  console.log('box.size', box.size);
  console.log('size.area', area);
  const bytes = (area + 7) >> 3;
  console.log('bytes', bytes);
  console.log('bits', bytes << 3);

  let a = [...binary(box)].map(n => !!n);
  // console.log('roi', a);
  console.log('bits', util.arrayToBitfield(a));

  function writeROI(i, rect) {
    let roi = mat(rect);
    let id = '0x' + (i + 0x20).toString(16).padStart(2, '0');
    let filename = fontName + '-' + id + '.png';
    cv.imwrite(filename, roi);
    console.log('write roi to', filename);
  }

  //  boxes.forEach((box, i) => writeROI(i, box));

  for(let box of boxes) cv.rectangle(mat, box.tl, box.br.sub(1, 1), [255, 0, 0], 1, cv.LINE_8);

  /* cv.resizeWindow('out', mat.cols * 2, mat.rows * 2);
  cv.imshow('out', mat);

  cv.waitKey(-1);*/
}

main(...process.argv.slice(1));

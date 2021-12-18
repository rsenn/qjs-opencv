import * as cv from 'opencv';
import * as util from 'util';
import { Window, MouseFlags, MouseEvents, Mouse, TextStyle, DrawText } from '../js/cvHighGUI.js';
import { Console } from 'console';

function main() {
  globalThis.console = new Console({
    inspectOptions: {
      colors: true,
      depth: 1,
      maxArrayLength: 30,
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

  let style = new TextStyle('MiscFixedSC613.ttf', 12);
  let str = util.range(0x20, 0x80).reduce((s, n) => s + String.fromCodePoint(n), '');

  let size = style.size(str);
  let rect = new cv.Rect(1, 1, size.width, size.height + 1);

  let step = rect.width / str.length;
  console.log('step', step);
  console.log('rect.height', rect.height);

  style.draw(mat, str, new cv.Point(-1, -1), colors[0], -1, cv.LINE_8);
  let roi = mat(rect);
  console.log('roi', roi);
  cv.imwrite('roi.png', roi);
  cv.imshow('out', mat);

  cv.waitKey(-1);
}

main();

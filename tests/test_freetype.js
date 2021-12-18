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
      depth: 2,
      maxArrayLength: Infinity,
      compact: 1
    }
  });

  /* cv.namedWindow('out');
  cv.resizeWindow('out', 640, 480);*/
  let mat;
  let colors = [
    [0, 0, 0],
    [255, 255, 255]
  ];

  cv.rectangle(mat, [0, 0], [639, 479], colors[1], cv.FILLED);

  let [fontFile = 'MiscFixedSC613.ttf', fontSize = 12] = args;

  console.log('fontFile', fontFile);
  let ext = path.extname(fontFile);

  let fontName = path.basename(fontFile, ext);
  let style = new TextStyle(fontFile, fontSize);
  let str = util.range(0x20, 0x80).reduce((s, n) => s + String.fromCodePoint(n), '');

  let size = style.size(str);
  let rect = new cv.Rect(1, 1, size.width, size.height + 2);

  let powerOf2 = n => Math.pow(2, Math.ceil(Math.log2(n)));

  let dim = new cv.Size(powerOf2(rect.width), powerOf2(rect.height));
  console.log('dim', dim);
  mat = new cv.Mat(dim, cv.CV_8UC3);
  cv.rectangle(mat, [0, 0], [mat.cols - 1, mat.rows - 1], colors[1], cv.FILLED);

  let step = Math.round(rect.width / str.length);
  console.log('step', step);
  console.log('rect.height', rect.height);
  let boxes = rect.hsplit(...util.range(0, rect.width, step));
  // console.log('boxes', boxes);

  style.draw(mat, str, new cv.Point(1, 0), colors[0], -1, cv.LINE_8);

  let gray = new cv.Mat();
  cv.cvtColor(mat, gray, cv.COLOR_BGR2GRAY);

  let roi = gray(rect);
  let rect2 = new cv.Rect(0, 0, roi.cols, roi.rows);

  console.log('gray', gray);
  console.log('roi', roi);
  console.log('cv.threshold', cv.threshold);
  let boxes2 = rect2.hsplit(...util.range(0, rect2.width, step));

  let binary = new cv.Mat();
  cv.threshold(roi, binary, 50, 255, cv.THRESH_BINARY_INV);

  console.log('binary', binary);
  //  console.log('binary.colRange()', binary.colRange());

  util.range(65, 100).forEach(code => {
    dumpChar(code);
  });

  function dumpChar(code) {
    const index = code < 32 ? 0 : code - 32;
    let box = new cv.Rect(boxes2[index]);

    console.log('box', box);
    box.y1 += 1;
    // box.x2 -= 1;
    console.log('box', box);

    let { area } = box.size;
    console.log('box.size', box.size);
    console.log('size.area', area);
    const bytes = (area + 7) >> 3;
    console.log('bytes', bytes);
    console.log('bits', bytes << 3);

    const RectReducer = list => {
      let first = new cv.Rect(...list.shift());
      console.log('first', first);
      return list.reduce((a, o) => {
        if('x' in o && 'width' in o) {
          a.x = Math.min(a.x, o.x);
          a.width = Math.max(a.x + a.width, o.x + o.width) - a.x;
        }
        if('y' in o && 'height' in o) {
          a.y = Math.min(a.y, o.y);
          a.height = Math.max(a.y + a.height, o.y + o.height) - a.y;
        }
        return a;
      }, first);
    };

    const PointReducer = list => {
      let first = new cv.Point(...list.shift());
      let rect = new cv.Rect(first.x, first.y, 0, 0);
      console.log('rect', rect);
      return list.reduce((a, o) => {
        if(a.x1 > o.x) a.x1 = o.x;
        if(a.x2 < o.x) a.x2 = o.x;
        if(a.y1 > o.y) a.y1 = o.y;
        if(a.y2 < o.y) a.y2 = o.y;
        return a;
      }, rect);
    };

    let a = [...binary(box)].map(n => !!n);

    let contour = new cv.Contour([...binary(box).entries()].filter(([coord, entry]) => entry != 0).map(p => new cv.Point(...p)));

    console.log('contour.boundingRect', contour.boundingRect);
    //console.log('contour',[...contour]);

    let pointList = [...binary(box).entries()].filter(([coord, entry]) => entry != 0).map(([coord, entry]) => new cv.Point(...coord));
    //console.log('pointList',  (pointList));
    console.log('PointReducer(pointList)', PointReducer(pointList));

    let bf = util.arrayToBitfield(a);
    console.log('bits', bf);
    console.log(
      `bits for '${String.fromCodePoint(code)}'`,
      '\n' +
        util
          .chunkArray(
            util.bitfieldToArray(bf).map(v => (v ? '\u2588\u2588' : '  ')),
            box.width
          )
          .map(row => row.join(''))
          .join('\n')
    );
  }

  //  console.log('bits',util.bitfieldToArray(bf).reduce((s,v) => s+(v|0).toString(), ''));

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

import { COLOR_BGR2GRAY, CV_8UC3, Contour, FILLED, LINE_8, LINE_AA, Mat, Point, Rect, Size, THRESH_BINARY_INV, WINDOW_NORMAL, cvtColor, drawRect, imshow, imwrite, namedWindow, resizeWindow, threshold, waitKey } from 'opencv';
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

  /* namedWindow('out');
  resizeWindow('out', 640, 480);*/
  let mat;
  let colors = [
    [0, 0, 0],
    [255, 255, 255]
  ];

  // drawRect(mat, [0, 0], [639, 479], colors[1], FILLED);

  let [fontFile = 'MiscFixedSC613.ttf', fontSize = 11] = args;

  console.log('fontFile', fontFile);
  let ext = path.extname(fontFile);

  let fontName = path.basename(fontFile, ext);
  let style = new TextStyle(fontFile, fontSize);
  let str = util.range(0x20, 0x80).reduce((s, n) => s + String.fromCodePoint(n), '');

  let size = style.size(str);
  let rect = new Rect(1, 1, size.width, size.height + 2);

  let powerOf2 = n => Math.pow(2, Math.ceil(Math.log2(n)));

  let dim = new Size(powerOf2(rect.width), powerOf2(Math.round(Math.min(rect.width / 1.7777), rect.height)));
  console.log('dim', dim);
  mat = new Mat(dim, CV_8UC3);
  drawRect(mat, [0, 0], [mat.cols - 1, mat.rows - 1], colors[1], FILLED);

  let step = Math.round(rect.width / str.length);
  console.log('step', step);
  console.log('rect.height', rect.height);
  let boxes = rect.hsplit(...util.range(0, rect.width, step));
  // console.log('boxes', boxes);

  style.draw(mat, str, new Point(1, 0), colors[0], -1, LINE_AA);

  let gray = new Mat();
  cvtColor(mat, gray, COLOR_BGR2GRAY);

  let roi = gray(rect);
  let rect2 = new Rect(0, 0, roi.cols, roi.rows);

  console.log('gray', gray);
  console.log('roi', roi);
  console.log('threshold', threshold);
  let boxes2 = rect2.hsplit(...util.range(0, rect2.width, step));

  let binary = new Mat();
  threshold(roi, binary, 50, 255, THRESH_BINARY_INV);

  namedWindow('out', WINDOW_NORMAL);
  let winsize = new Size(rect.width, Math.max(mat.cols / 1.77777, rect.height));
  console.log('winsize', winsize);
  resizeWindow('out', ...mat.size);
  imshow('out', mat);
  waitKey(-1);

  console.log('binary', binary);
  //  console.log('binary.colRange()', binary.colRange());

  util.range(0x30, 0x39).forEach(code => {
    dumpChar(code);
  });

  function dumpChar(code) {
    const index = code < 32 ? 0 : code - 32;
    let box = new Rect(boxes2[index]);

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
      let first = new Rect(...list.shift());
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
      let first = new Point(...list.shift());
      let rect = new Rect(first.x, first.y, 0, 0);
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

    let contour = new Contour(
      [...binary(box).entries()].filter(([coord, entry]) => entry != 0).map(p => new Point(...p))
    );

    console.log('contour.boundingRect', contour.boundingRect);
    //console.log('contour',[...contour]);

    let pointList = [...binary(box).entries()]
      .filter(([coord, entry]) => entry != 0)
      .map(([coord, entry]) => new Point(...coord));
    //console.log('pointList',  (pointList));
    console.log('PointReducer(pointList)', PointReducer(pointList));

    let bf = util.arrayToBitfield(a);
    console.log('bits', console.config({ numberBase: 2 }), new Uint8Array(bf));
    console.log(
      `bits for '${String.fromCodePoint(code)}'`,
      '\n\x1b[48;5;232m' +
        util
          .chunkArray(
            util.bitfieldToArray(bf).map(v => (v ? '\u2588\u2588' : '  ')),
            box.width
          )
          .map(row => row.join(''))
          .join('\n') +
        '\x1b[0m'
    );
  }

  //  console.log('bits',util.bitfieldToArray(bf).reduce((s,v) => s+(v|0).toString(), ''));
  function Grayscale(src) {
    let m = new Mat();
    cvtColor(src, m, COLOR_BGR2GRAY);
    return m;
  }

  /*  let hist = new Mat();
    calcHist([dst], 1, 0, new Mat(), hist, 1,  [255],[0,256]);
    console.log('hist', hist);*/

  function writeROI(i, rect) {
    let roi = mat(rect);
    let id = '0x' + (i + 0x20).toString(16).padStart(2, '0');
    let filename = fontName + '-' + id + '.png';
    imwrite(filename, roi);
    //console.log('write roi to', filename);
  }

  function writeFile(filename,s) {
    let f=std.open(filename,'w+');
    f.puts(s);
    f.close();
  }

  function getRow(mat, r) {
    return mat.row(r).array;
  }

  function* rowIterator(mat) {
    for(let i = 0; i < mat.rows; i++) yield getRow(mat,i);
  }
function outputBytes(mat) {
  return [...rowIterator(mat)].map(a => [...a.values()]);
}
function toSource(obj) {
  return inspect(obj, { reparseable: 1,  colors:false ,compact: 3, numberBase: 16});
}

  //boxes.forEach((box, i) => writeROI(i, box));
  let i = 0;
  gray = Grayscale(mat);
let font=[];
  for(let box of boxes) {
    let m = gray(box);
    m.xor(0xff);

    const { rows, cols, type, channels, depth } = m;
    console.log('m', { rows, cols, type, channels, depth });
    font.push([i+0x20, outputBytes(m)]);
    
    let r = [...rowIterator(m)];
    //   console.log(i, console.config({maxArrayLength:10}), rows);

    writeROI(i++, box);
    drawRect(mat, box.tl, box.br.sub(1, 1), [255, 0, 0], 1, LINE_8);
  }

writeFile('output.js', toSource(font));



  imshow('out', mat);
  waitKey(-1);
}

main(...process.argv.slice(1));

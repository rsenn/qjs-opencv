import * as cv from 'opencv';
import { Size, Point, Draw } from 'opencv';
import { BitsToNames } from './cvUtils.js';

export const MouseEvents = [
  'EVENT_MOUSEMOVE',
  'EVENT_LBUTTONDOWN',
  'EVENT_RBUTTONDOWN',
  'EVENT_MBUTTONDOWN',
  'EVENT_LBUTTONUP',
  'EVENT_RBUTTONUP',
  'EVENT_MBUTTONUP',
  'EVENT_LBUTTONDBLCLK',
  'EVENT_RBUTTONDBLCLK',
  'EVENT_MBUTTONDBLCLK',
  'EVENT_MOUSEWHEEL',
  'EVENT_MOUSEHWHEEL'
].reduce((acc, name) => ({ ...acc, [cv[name]]: name }), {});

export const MouseFlags = [
  'EVENT_FLAG_LBUTTON',
  'EVENT_FLAG_RBUTTON',
  'EVENT_FLAG_MBUTTON',
  'EVENT_FLAG_CTRLKEY',
  'EVENT_FLAG_SHIFTKEY',
  'EVENT_FLAG_ALTKEY'
].reduce((acc, name) => ({ ...acc, [name]: cv[name] }), {});

export const Mouse = {
  printEvent: (() => {
    return event => MouseEvents[event].replace(/EVENT_/, '');
  })(),
  printFlags: (() => {
    const toks = BitsToNames(MouseFlags, name => name.replace(/EVENT_FLAG_/, ''));

    return flags => [...toks(flags)];
  })()
};

export class Window {
  constructor(name, flags = cv.WINDOW_NORMAL) {
    this.name = name;
    this.flags = flags;

    cv.namedWindow(this.name, this.flags);
  }

  move(x, y) {
    cv.moveWindow(this.name, x, y);
  }

  resize(...args) {
    //console.log("Window.resize", ...args);
    let size = new Size(...args);
    cv.resizeWindow(this.name, ...size);
  }

  /* prettier-ignore */ get imageRect() {
    return cv.getWindowImageRect(this.name);
  }

  get(propId) {
    return cv.getWindowProperty(this.name, propId);
  }
  set(propId, value) {
    cv.setWindowProperty(this.name, propId, value);
  }

  setTitle(title) {
    this.title = title;
    cv.setWindowTitle(this.name, title);
  }

  setMouseCallback(fn) {
    console.log('Window.setMouseCallback', fn);
    cv.setMouseCallback(this.name, (event, x, y, flags) => {
      //console.log("MouseCallback", {event,x,y,flags});
      fn.call(this, event, x, y, flags);
    });
  }

  show(mat) {
    cv.imshow(this.name, mat);
  }

  valueOf() {
    return this.name;
  }
}

export function TextStyle(fontFace = cv.FONT_HERSHEY_PLAIN, fontScale = 1.0, thickness = 1) {
  Object.assign(this, { fontFace, fontScale, thickness });
}

Object.assign(TextStyle.prototype, {
  size(text, fn = y => {}) {
    const { fontFace, fontScale, thickness } = this;
    let baseY;
    let size = new Size(...Draw.textSize(text, fontFace, fontScale, thickness, y => (baseY = y)));

    fn(baseY);

    size.y = baseY;
    return size;
  },

  draw(mat, text, pos, color, lineThickness, lineType) {
    const { fontFace, fontScale, thickness } = this;
    const args = [
      mat,
      text,
      pos,
      fontFace,
      fontScale,
      color ?? 0xffffff,
      lineThickness ?? thickness,
      lineType ?? cv.LINE_AA
    ];
    //console.log('TextStyle draw(', ...args.reduce((acc, arg) => (acc.length ? [...acc, ',', arg] : [arg]), []), ')');
    Draw.text(...args);
  }
});

const palette16 = [
  0x000000, 0xa00000, 0x00a000, 0xa0a000, 0x0000a0, 0xa000a0, 0x00a0a0, 0xc0c0c0, 0xa0a0a0, 0xff0000, 0x00ff00,
  0xffff00, 0x0000ff, 0xff00ff, 0x00ffff, 0xffffff
];

export function DrawText(dst, text, color, fontFace, fontSize = 13) {
  let c = color;
  let font =
    typeof fontFace == 'object' && fontFace != null && fontFace instanceof TextStyle
      ? fontFace
      : new TextStyle(fontFace, fontSize, -1);
  let lines = [...text.matchAll(/(\x1b[^a-z]*[a-z]|\n|[^\x1b\n]*)/g)].map(m => m[0]);
  let baseY;
  let size = font.size('yP', y => (baseY = y));
  let start = new Point(size.width / text.length, baseY - 3);
  let pos = new Point(start);
  let incY = (baseY || 2) + size.height + 3;

  for(let line of lines) {
    if(line == '\n') {
      pos.y += incY;
      pos.x = start.x;
      continue;
    } else if(line.startsWith('\x1b')) {
      let ansi = [...line.matchAll(/([0-9]+|[a-z])/g)].map(m => (isNaN(+m[0]) ? m[0] : +m[0]));
      if(ansi[ansi.length - 1] == 'm') {
        let n;
        for(let code of ansi.slice(0, -1)) {
          if(code == 0) continue;
          if(code == 1) n = (n | 0) + 8;
          else if(code >= 30) n = n | 0 | (code - 30);
        }
        if(n === undefined) c = color;
        else c = palette16[n];
      }
      continue;
    }
    size = font.size(line);
    font.draw(dst, line, pos, c, -1, cv.LINE_AA);
    pos.x += size.width;
  }
}

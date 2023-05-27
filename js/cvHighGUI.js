import { waitKey, waitKeyEx, selectROI, Size, Point, Draw, FILLED, FONT_HERSHEY_PLAIN, getScreenResolution, getWindowImageRect, getWindowProperty, imshow, LINE_AA, moveWindow, namedWindow, resizeWindow, destroyWindow, setMouseCallback, setWindowProperty, setWindowTitle, WINDOW_NORMAL, EVENT_MOUSEMOVE, EVENT_LBUTTONDOWN, EVENT_RBUTTONDOWN, EVENT_MBUTTONDOWN, EVENT_LBUTTONUP, EVENT_RBUTTONUP, EVENT_MBUTTONUP, EVENT_LBUTTONDBLCLK, EVENT_RBUTTONDBLCLK, EVENT_MBUTTONDBLCLK, EVENT_MOUSEWHEEL, EVENT_MOUSEHWHEEL, EVENT_FLAG_LBUTTON, EVENT_FLAG_RBUTTON, EVENT_FLAG_MBUTTON, EVENT_FLAG_CTRLKEY, EVENT_FLAG_SHIFTKEY, EVENT_FLAG_ALTKEY } from 'opencv';
import { BitsToNames } from './cvUtils.js';

export const MouseEvents = {
  EVENT_MOUSEMOVE,
  EVENT_LBUTTONDOWN,
  EVENT_RBUTTONDOWN,
  EVENT_MBUTTONDOWN,
  EVENT_LBUTTONUP,
  EVENT_RBUTTONUP,
  EVENT_MBUTTONUP,
  EVENT_LBUTTONDBLCLK,
  EVENT_RBUTTONDBLCLK,
  EVENT_MBUTTONDBLCLK,
  EVENT_MOUSEWHEEL,
  EVENT_MOUSEHWHEEL
};
export const MouseFlags = {
  EVENT_FLAG_LBUTTON,
  EVENT_FLAG_RBUTTON,
  EVENT_FLAG_MBUTTON,
  EVENT_FLAG_CTRLKEY,
  EVENT_FLAG_SHIFTKEY,
  EVENT_FLAG_ALTKEY
};

export const Mouse = {
  printEvent: (() => {
    return event => MouseEvents[event].replace(/EVENT_/, '');
  })(),
  printFlags: (() => {
    const toks = BitsToNames(MouseFlags, name => name.replace(/EVENT_FLAG_/, ''));

    return flags => [...toks(flags)];
  })()
};

export class Screen {
  static size() {
    return getScreenResolution();
  }
}

export class Window {
  constructor(name, flags = WINDOW_NORMAL) {
    this.flags = flags;
    if(typeof name == 'string') this.#create(name);
  }

  #create(name = 'default') {
    if(!this.name) {
      this.name = name;

      namedWindow(this.name, this.flags);
    }
  }

  #handleKey(scancode) {
    const mods = scancode >> 16;
    const keycode = scancode & 0xffff;
    const ev = { key: String.fromCharCode(keycode & 0xff), keycode, scancode, mods };
    if(typeof this.onkey == 'function') this.onkey(ev);
  }

  update(waitFor = 10) {
    let key, t;
    const deadline = (t = Date.now()) + waitFor;
    do {
      if((key = waitKeyEx(Math.max(1, deadline - t))) == -1) break;

      this.#handleKey(key);
    } while((t = Date.now()) < deadline);
    //console.log('Window.update', { waitFor, key, t: deadline - t });
  }

  move(...args) {
    let pos = new Point(...args);
    moveWindow(this.name, pos.x, pos.y);

    this.update();
  }

  resize(...args) {
    //console.log("Window.resize", ...args);
    let size = new Size(...args);
    resizeWindow(this.name, ...size);

    this.update();
    return size;
  }

  align(n = 0) {
    let s = Screen.size();
    let rect = this.imageRect;
    let dim = new Size(rect);
    let { x, y } = dim.align(s, n);

    this.update();
    return this.move(x, y);
  }

  /* prettier-ignore */ get imageRect() {
    return getWindowImageRect(this.name);
  }

  get(propId) {
    return getWindowProperty(this.name, propId);
  }
  set(propId, value) {
    setWindowProperty(this.name, propId, value);

    this.update();
  }

  setTitle(title) {
    this.title = title;
    setWindowTitle(this.name, title);

    this.update();
  }

  setMouseCallback(fn) {
    console.log('Window.setMouseCallback', fn);
    setMouseCallback(this.name, (event, x, y, flags) => {
      //console.log("MouseCallback", {event,x,y,flags});
      fn.call(this, event, x, y, flags);
    });

    this.update();
  }

  show(mat) {
    this.mat = mat;
    imshow(this.name, mat);

    //this.update(50);
  }

  close() {
    if(this.name) {
      destroyWindow(this.name);
      this.name = undefined;
    }
    this.update();
  }

  valueOf() {
    return this.name;
  }

  selectROI(mat, showCrosshair = true, fromCenter = false) {
    return selectROI(this.name, mat ?? this.mat, showCrosshair, fromCenter);
  }
}

export function TextStyle(fontFace = FONT_HERSHEY_PLAIN, fontScale = 1.0, thickness = -1) {
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

  draw(mat, text, pos, color = [255, 255, 255, 255], lineThickness = FILLED, lineType = LINE_AA) {
    const { fontFace, fontScale, thickness } = this;
    Draw.text(mat, text, pos, fontFace, fontScale, color, lineThickness, lineType);
  }
});

const palette16 = [0x000000, 0xa00000, 0x00a000, 0xa0a000, 0x0000a0, 0xa000a0, 0x00a0a0, 0xc0c0c0, 0xa0a0a0, 0xff0000, 0x00ff00, 0xffff00, 0x0000ff, 0xff00ff, 0x00ffff, 0xffffff];

export function DrawText(dst, text, color, fontFace, fontSize = 13) {
  let c = color;
  let font = typeof fontFace == 'object' && fontFace != null && fontFace instanceof TextStyle ? fontFace : new TextStyle(fontFace, fontSize, -1);
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
    font.draw(dst, line, pos, c, -1, LINE_AA);
    pos.x += size.width;
  }
}

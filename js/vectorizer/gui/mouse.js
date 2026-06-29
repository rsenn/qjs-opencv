// gui/mouse.js
//
// HighGUI delivers mouse activity through setMouseCallback. We push events into
// a queue and the controller drains it each loop tick, so handlers run on the
// main loop rather than inside the callback. This keeps the MVC flow single
// threaded and predictable.

import { setMouseCallback, EVENT_LBUTTONDOWN, EVENT_LBUTTONUP, EVENT_MOUSEMOVE, EVENT_MOUSEWHEEL } from 'opencv.so';

export const MouseEvent = {
  DOWN: EVENT_LBUTTONDOWN,
  UP: EVENT_LBUTTONUP,
  MOVE: EVENT_MOUSEMOVE,
  WHEEL: EVENT_MOUSEWHEEL,
};

export class Mouse {
  constructor(windowName) {
    this.queue = [];
    this.x = 0;
    this.y = 0;
    this.down = false;
    setMouseCallback(windowName, (event, x, y, flags) => {
      this.x = x;
      this.y = y;
      if(event === EVENT_LBUTTONDOWN) this.down = true;
      if(event === EVENT_LBUTTONUP) this.down = false;
      this.queue.push({ event, x, y, flags });
    });
  }

  drain() {
    const q = this.queue;
    this.queue = [];
    return q;
  }
}

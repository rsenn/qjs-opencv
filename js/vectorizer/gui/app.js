/*
 * gui/app.js
 *
 * The Controller in our MVC, plus the wizard state machine that sequences the
 * four stages. It owns the single HighGUI window, the input snapshot and the
 * shared Hud. Stages are pure View+local-controller objects: App calls
 * stage.draw(this) each frame and the stage mutates the Model; any per-method
 * parameters are drawn as cvWidgets trackbar widgets by the stage itself
 * (see stage3-process.js), not by App - unlike native HighGUI trackbars,
 * these are just draw calls, so the window is created once and never rebuilt.
 */

import {
  namedWindow, destroyWindow, imshow, waitKeyEx, getWindowImageRect, setWindowProperty, resizeWindow,
  WINDOW_NORMAL, WINDOW_FULLSCREEN, WND_PROP_FULLSCREEN,
} from 'opencv';
import * as os from 'os';

import { Stage } from '../core/model.js';
import { Canvas, Palette } from './canvas.js';
import { Hud } from '../../cvWidgets.js';
import { Mouse, MouseEvent, getMouseWheelDelta } from './mouse.js';

import { LoadStage } from './stages/stage1-load.js';
import { AssignStage } from './stages/stage2-assign.js';
import { ProcessStage } from './stages/stage3-process.js';
import { ProjectStage } from './stages/stage4-project.js';

const WIN = 'qjs-vectorizer';
const TABS = ['1 · Load', '2 · Assign', '3 · Process', '4 · Project'];
const MIN_W = 640, MIN_H = 480;
// GDK_KEY_F11 (X11 keysym 0xffc8) - waitKeyEx() returns the raw keyval for
// keys with no special-cased low code (see icvOnKeyPress in window_gtk.cpp);
// plain waitKey() would mask this down to 0xc8 and collide with other keys.
const KEY_F11 = 0xffc8;

export class App {
  constructor({ model, pipeline, registry }) {
    this.model = model;
    this.pipeline = pipeline;
    this.registry = registry;

    this.W = 1180;
    this.H = 740;
    this.canvas = new Canvas(this.W, this.H);
    this.hud = new Hud();
    this.events = [];
    this.onExport = null;

    this.stages = {
      [Stage.LOAD]: new LoadStage(),
      [Stage.ASSIGN]: new AssignStage(),
      [Stage.PROCESS]: new ProcessStage(),
      [Stage.PROJECT]: new ProjectStage(),
    };

    this.fullscreen = false;
    this.wheel = null;

    // WINDOW_NORMAL (resizable) is required both for the user to be able to
    // resize the window at all, and for setWindowProperty(WND_PROP_FULLSCREEN)
    // to work - the GTK backend's setModeWindow_() refuses to toggle
    // fullscreen on a WINDOW_AUTOSIZE window (see window_gtk.cpp).
    namedWindow(WIN, WINDOW_NORMAL);
    // WINDOW_NORMAL doesn't auto-size to the first imshow() like AUTOSIZE
    // did - without this the window opens at the backend's small default
    // and our own resize-detection loop then shrinks the canvas to match.
    resizeWindow(WIN, this.W, this.H);
    this.mouse = new Mouse(WIN);
    this._enterStage(this.model.stage);
  }

  // Recreate the canvas at the window's current size so drawing is crisp at
  // 1:1 - GTK's CvImageWidget itself rescales whatever Mat we imshow() to
  // fit the window, and re-rasterizing our own UI at that resolution avoids
  // the blur that scaling a fixed-size Mat up/down would otherwise produce.
  _resize(w, h) {
    this.W = Math.max(MIN_W, w);
    this.H = Math.max(MIN_H, h);
    this.canvas = new Canvas(this.W, this.H);
  }

  toggleFullscreen() {
    this.fullscreen = !this.fullscreen;
    setWindowProperty(WIN, WND_PROP_FULLSCREEN, this.fullscreen ? WINDOW_FULLSCREEN : WINDOW_NORMAL);
  }

  get stage() {
    return this.stages[this.model.stage];
  }

  _enterStage(s) {
    this.model.setStage(s);
    if(this.stage.enter) this.stage.enter(this);
  }

  goTo(s) {
    if(s < Stage.LOAD || s > Stage.PROJECT) return;
    if(this.stage['leave']) this.stage['leave'](this);
    this._enterStage(s);
  }

  next() {
    if(this.pipeline.canAdvance(this.model.stage)) this.goTo(this.model.stage + 1);
  }
  back() {
    this.goTo(this.model.stage - 1);
  }

  async run() {
    let running = true;
    while(running) {
      // --- input snapshot ---
      this.events = this.mouse.drain();
      const clicks = this.events.filter(e => e.event === MouseEvent.DOWN).map(e => ({ x: e.x, y: e.y }));
      const wheelEvent = this.events.findLast(e => e.event === MouseEvent.WHEEL);
      this.wheel = wheelEvent ? { delta: getMouseWheelDelta(wheelEvent.flags) } : null;
      this.hud.frame(clicks, { x: this.mouse.x, y: this.mouse.y }, this.mouse.down, this.wheel);

      // --- draw ---
      this.canvas.clear();
      this._drawChrome();
      this.stage.draw(this);

      imshow(WIN, this.canvas.mat);
      const key = waitKeyEx(16);
      if(key === 27)
        running = false; // ESC
      else if(key === 9)
        this.next(); // TAB -> next
      else if(key === KEY_F11)
        this.toggleFullscreen();
      else if(this.stage['onKey']) this.stage['onKey'](this, key);

      // the WM's X (close) button doesn't produce a key event - GTK's
      // "delete-event" tears the window down internally (icvOnClose ->
      // icvDeleteWindow_ in window_gtk.cpp) with no signal back to us.
      // getWindowProperty(WND_PROP_VISIBLE) can't detect this - the GTK
      // backend never implements it, so it always returns -1 regardless of
      // real state (confirmed empirically; see BUGS). getWindowImageRect()
      // DOES detect it: cvGetWindowRect_GTK throws StsNullPtr once the
      // window is gone, which is what we probe here. Reuse the same call to
      // detect a live resize (dragging the window edges) and rebuild the
      // canvas at the new size.
      if(running) {
        try {
          const rect = getWindowImageRect(WIN);
          const w = Math.max(MIN_W, rect.width), h = Math.max(MIN_H, rect.height);
          if(w !== this.W || h !== this.H) this._resize(w, h);
        } catch(_) {
          running = false;
        }
      }

      // pump qjs event loop so os.Worker messages (progress/done) fire.
      await os.sleepAsync(0);
    }
    try {
      destroyWindow(WIN);
    } catch(_) {}
  }

  // Shared chrome: stage tabs + Back/Next/Export.
  _drawChrome() {
    const cv = this.canvas,
      hud = this.hud;
    cv.panel(0, 0, this.W, 44, Palette.panel2);
    const tabW = 150;
    for(let i = 0; i < TABS.length; i++) {
      const x = 12 + i * (tabW + 6);
      const active = i === this.model.stage;
      // Any past/current tab jumps straight there. The one tab right after
      // the current stage acts like the Next button (same canAdvance gate) -
      // tabs further ahead than that stay disabled, so you can't skip steps.
      const isNext = i === this.model.stage + 1;
      const clickable = i <= this.model.stage || (isNext && this.pipeline.canAdvance(this.model.stage));
      if(hud.button(cv, x, 7, tabW, 30, TABS[i], { active, disabled: !clickable })) {
        if(i <= this.model.stage) this.goTo(i);
        else if(isNext) this.next();
      }
    }
    // nav buttons on the right
    if(hud.button(cv, this.W - 240, 7, 70, 30, '< Back', { disabled: this.model.stage === 0 })) this.back();
    const canNext = this.pipeline.canAdvance(this.model.stage) && this.model.stage < Stage.PROJECT;
    if(hud.button(cv, this.W - 162, 7, 70, 30, 'Next >', { disabled: !canNext, active: canNext })) this.next();
    if(hud.button(cv, this.W - 84, 7, 72, 30, 'Export', { active: this.model.stage === Stage.PROJECT })) this.export();
    cv.line(0, 44, this.W, 44, Palette.line, 1);
  }

  export() {
    const svg = this.pipeline.composeSVG();
    if(this.onExport) this.onExport(svg);
  }
}

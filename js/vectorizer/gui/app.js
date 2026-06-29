// gui/app.js
//
// The Controller in our MVC, plus the wizard state machine that sequences the
// four stages. It owns the single HighGUI window, the input snapshot, the
// shared Hud, and the trackbar manager. Stages are pure View+local-controller
// objects: App calls stage.draw(this) each frame and the stage mutates the
// Model and reads/writes trackbars through App helpers.

import { namedWindow, destroyWindow, imshow, waitKey, WINDOW_AUTOSIZE } from 'opencv.so';

import { Stage } from '../core/model.js';
import { Canvas, Palette } from './canvas.js';
import { Hud } from './widgets.js';
import { Mouse, MouseEvent } from './mouse.js';
import { Trackbars } from './trackbars.js';

import { LoadStage } from './stages/stage1-load.js';
import { AssignStage } from './stages/stage2-assign.js';
import { ProcessStage } from './stages/stage3-process.js';
import { ProjectStage } from './stages/stage4-project.js';

const WIN = 'qjs-vectorizer';
const TABS = ['1 · Load', '2 · Assign', '3 · Process', '4 · Project'];

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

    this.stages = {
      [Stage.LOAD]: new LoadStage(),
      [Stage.ASSIGN]: new AssignStage(),
      [Stage.PROCESS]: new ProcessStage(),
      [Stage.PROJECT]: new ProjectStage(),
    };

    this.recreateWindow();
    this._enterStage(this.model.stage);
  }

  // (Re)create the window — the only way to clear HighGUI trackbars. Rebinds
  // the mouse callback since destroying the window drops it.
  recreateWindow() {
    try {
      destroyWindow(WIN);
    } catch(_) {}
    namedWindow(WIN, WINDOW_AUTOSIZE);
    this.mouse = new Mouse(WIN);
    this.trackbars = new Trackbars(WIN);
  }

  get stage() {
    return this.stages[this.model.stage];
  }

  _enterStage(s) {
    this.model.setStage(s);
    this.recreateWindow();
    if(this.stage.enter) this.stage.enter(this);
  }

  goTo(s) {
    if(s < Stage.LOAD || s > Stage.PROJECT) return;
    if(this.stage.leave) this.stage.leave(this);
    this._enterStage(s);
  }

  next() {
    if(this.pipeline.canAdvance(this.model.stage)) this.goTo(this.model.stage + 1);
  }
  back() {
    this.goTo(this.model.stage - 1);
  }

  run() {
    let running = true;
    while(running) {
      // --- input snapshot ---
      this.events = this.mouse.drain();
      const clicks = this.events.filter(e => e.event === MouseEvent.DOWN).map(e => ({ x: e.x, y: e.y }));
      this.hud.frame(clicks, { x: this.mouse.x, y: this.mouse.y }, this.mouse.down);

      // --- draw ---
      this.canvas.clear();
      this._drawChrome();
      this.stage.draw(this);

      // --- trackbar polling -> stage ---
      const delta = this.trackbars.poll();
      if(delta && this.stage.onParams) this.stage.onParams(this, delta);

      imshow(WIN, this.canvas.mat);
      const key = waitKey(16);
      if(key === 27)
        running = false; // ESC
      else if(key === 9)
        this.next(); // TAB -> next
      else if(this.stage.onKey) this.stage.onKey(this, key);
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
      const reachable = i <= this.model.stage || this.pipeline.canAdvance(this.model.stage);
      if(hud.button(cv, x, 7, tabW, 30, TABS[i], { active, disabled: i > this.model.stage })) if (i <= this.model.stage) this.goTo(i);
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

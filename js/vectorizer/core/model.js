// core/model.js
//
// The single shared Model in our MVC. It owns the whole pipeline document and
// emits change events; every stage's View renders from it and every
// Controller mutates it. It stores frame handles (cv.Mat) opaquely — it never
// imports OpenCV, which keeps the model layer testable and decoupled.

import { Emitter } from './events.js';
import { identity } from './geometry.js';

let _seq = 0;
const uid = p => `${p}${++_seq}`;

export const Stage = { LOAD: 0, ASSIGN: 1, PROCESS: 2, PROJECT: 3 };

export class Model extends Emitter {
  constructor() {
    super();
    this.stage = Stage.LOAD;

    // Stage 1 output: sources discovered on the CLI, and the frames the user
    // selected from them. A "frame" is { id, sourceId, label, mat, w, h }.
    this.sources = []; // { id, kind:'image'|'video', uri, frameCount }
    this.frames = []; // selected frames (mat = opaque cv.Mat handle)

    // Stage 2 output: frameId -> methodId
    this.assignments = new Map();

    // Stage 3 output: frameId -> { params, vectorData }
    this.results = new Map();

    // Stage 4 output: frameId -> placement { H, opacity, quad }
    this.placements = new Map();

    // Output canvas
    this.canvas = { width: 1280, height: 720, background: '#ffffff' };
  }

  setStage(s) {
    this.stage = s;
    this.emit('stage', s);
  }

  addSource(kind, uri, frameCount = 1) {
    const src = { id: uid('src'), kind, uri, frameCount };
    this.sources.push(src);
    this.emit('sources', this.sources);
    return src;
  }

  addFrame(sourceId, mat, label, w, h) {
    const fr = { id: uid('fr'), sourceId, mat, label, w, h };
    this.frames.push(fr);
    this.emit('frames', this.frames);
    return fr;
  }

  removeFrame(id) {
    this.frames = this.frames.filter(f => f.id !== id);
    this.assignments.delete(id);
    this.results.delete(id);
    this.placements.delete(id);
    this.emit('frames', this.frames);
  }

  assign(frameId, methodId) {
    this.assignments.set(frameId, methodId);
    this.results.delete(frameId); // params/result invalidated
    this.emit('assign', { frameId, methodId });
  }

  setResult(frameId, params, vectorData) {
    this.results.set(frameId, { params, vectorData });
    this.emit('result', { frameId, params, vectorData });
  }

  setPlacement(frameId, placement) {
    const cur = this.placements.get(frameId) || { H: identity(), opacity: 1, quad: null };
    this.placements.set(frameId, Object.assign(cur, placement));
    this.emit('placement', { frameId, placement: this.placements.get(frameId) });
  }

  frame(id) {
    return this.frames.find(f => f.id === id);
  }

  // Frames that have a usable vectorization result — the input to stage 4.
  vectorizedFrames() {
    return this.frames.filter(f => this.results.has(f.id));
  }
}

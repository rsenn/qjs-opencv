// gui/stages/stage1-load.js
//
// Stage 1 — LOAD. Shows every discovered source as a thumbnail tile. Clicking
// an image toggles it into the frame set. Clicking a video focuses it and adds
// a scrub trackbar so individual frames can be picked and added.

import * as path from 'path';
import { Palette } from '../canvas.js';

export class LoadStage {
  constructor() { this.focused = null; this.scrub = 0; this.scrubMat = null; }

  enter(app) {
    // If a video is focused, expose a scrub slider for it.
    const src = app.model.sources.find((s) => s.id === this.focused);
    if (src && src.kind === 'video') {
      app.trackbars.add(
        { key: 'frame', label: 'video frame', type: 'int', min: 0, max: Math.max(0, src.frameCount - 1), step: 1, default: this.scrub },
        this.scrub,
      );
      this._loadScrub(app, src);
    }
  }

  onParams(app, delta) {
    if (delta.frame != null) {
      this.scrub = delta.frame;
      const src = app.model.sources.find((s) => s.id === this.focused);
      if (src) this._loadScrub(app, src);
    }
  }

  _loadScrub(app, src) {
    this.scrubMat = app.pipeline.loader.extractFrame(src, this.scrub);
  }

  draw(app) {
    const cv = app.canvas, hud = app.hud, m = app.model;
    cv.text('Select frames to vectorize  ·  click images to toggle, videos to scrub',
      16, 66, Palette.textDim, 0.46);

    const focusedSrc = m.sources.find((s) => s.id === this.focused);
    const gridW = focusedSrc && focusedSrc.kind === 'video' ? app.W - 380 : app.W - 20;

    // --- source grid ---
    const tw = 150, th = 100, pad = 14;
    const cols = Math.max(1, Math.floor((gridW - 16) / (tw + pad)));
    let i = 0;
    for (const src of m.sources) {
      const col = i % cols, row = (i / cols) | 0;
      const x = 16 + col * (tw + pad), y = 84 + row * (th + 34);
      const selected = src.kind === 'image'
        ? m.frames.some((f) => f.sourceId === src.id)
        : src.id === this.focused;
      if (src._preview) cv.paste(src._preview, x, y, tw, th);
      else cv.rect(x, y, tw, th, Palette.panel, true);
      const tag = src.kind === 'video' ? `▶ ${path.basename(src.uri)} (${src.frameCount})` : path.basename(src.uri);
      if (hud.tile(cv, x, y, tw, th, tag, selected)) this._onTileClick(app, src);
      i++;
    }

    // --- video scrub panel ---
    if (focusedSrc && focusedSrc.kind === 'video') {
      const px = app.W - 350, pw = 338;
      cv.panel(px, 56, pw, app.H - 70, Palette.panel);
      cv.text(`${path.basename(focusedSrc.uri)}  ·  frame ${this.scrub}`, px + 10, 80, Palette.text, 0.46);
      if (this.scrubMat) cv.paste(this.scrubMat, px + 10, 92, pw - 20, 250);
      if (hud.button(cv, px + 10, 360, pw - 20, 34, `Add frame ${this.scrub}`, { active: true }))
        this._addVideoFrame(app, focusedSrc);
    }

    cv.text(`${m.frames.length} frame(s) selected`, 16, app.H - 16, Palette.textDim, 0.46);
  }

  _onTileClick(app, src) {
    const m = app.model;
    if (src.kind === 'image') {
      const existing = m.frames.find((f) => f.sourceId === src.id);
      if (existing) { m.removeFrame(existing.id); return; }
      const mat = app.pipeline.loader.extractFrame(src, 0);
      m.addFrame(src.id, mat, path.basename(src.uri), src._w || (mat.cols ?? mat.width), src._h || (mat.rows ?? mat.height));
    } else {
      this.focused = src.id; this.scrub = 0;
      app.goTo(app.model.stage);   // re-enter stage to (re)build scrub trackbar
    }
  }

  _addVideoFrame(app, src) {
    const mat = app.pipeline.loader.extractFrame(src, this.scrub);
    const w = mat.cols ?? mat.width, h = mat.rows ?? mat.height;
    const fr = app.model.addFrame(src.id, mat, `${path.basename(src.uri)}#${this.scrub}`, w, h);
    fr.index = this.scrub;
  }
}

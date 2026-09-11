// cv/loader.js
//
// Stage-1 collaborator. Discovers image/video sources from CLI paths (files or
// directories) and extracts individual frames as cv.Mat. The pipeline only sees
// the abstract { discover, extractFrame } interface, so this could be swapped
// for a PDF-page loader without touching the pipeline.

import * as fs from 'fs';
import * as path from 'path';
import { Mat, Size, imread, resize, VideoCapture, INTER_AREA, CAP_PROP_FRAME_COUNT, CAP_PROP_POS_FRAMES } from 'opencv.so';

const IMAGE_EXT = new Set(['.png', '.jpg', '.jpeg', '.bmp', '.webp', '.tif', '.tiff', '.gif', '.ppm', '.pgm']);
const VIDEO_EXT = new Set(['.mp4', '.avi', '.mov', '.mkv', '.webm', '.m4v', '.mpg', '.mpeg']);

// A modern phone/camera frame (e.g. 4032x3024) is massive overkill for a
// vector-art source: stage 3 re-runs the assigned method on every trackbar
// tweak, and the output canvas (see vectorizer.js's --width/--height,
// default 1280x720) is far smaller anyway. Left at full resolution, a
// detail-heavy method like Canny+Contours produces tens of thousands of
// tiny shapes, and both the vectorize call and stage3-process.js's
// synchronous preview render (drawVectorData) scale with that shape count -
// together they measured ~12s on a 4032x3024 photo, which reads as a hang
// since nothing in the UI distinguishes "slow" from "stuck". Downscaling
// once here, at load time, keeps every downstream stage (vectorize,
// preview render, SVG export) working on a consistently-sized Mat.
const MAX_DIM = 1600;

function capResolution(mat) {
  const long = Math.max(mat.cols, mat.rows);
  if (long <= MAX_DIM) return mat;
  const scale = MAX_DIM / long;
  const out = new Mat();
  resize(mat, out, new Size(Math.round(mat.cols * scale), Math.round(mat.rows * scale)), 0, 0, INTER_AREA);
  return out;
}

export class Loader {
  // Expand args (dirs -> their files) into source descriptors.
  discover(uris) {
    const files = [];
    for(const u of uris) {
      if(!fs.existsSync(u)) {
        console.log('skip (missing):', u);
        continue;
      }
      if(path.isDirectory(u)) {
        for(const e of fs.readdirSync(u).sort()) files.push(path.join(u, e));
      } else {
        files.push(u);
      }
    }
    const sources = [];
    for(const f of files) {
      const ext = path.extname(f).toLowerCase();
      if(IMAGE_EXT.has(ext)) sources.push({ kind: 'image', uri: f, frameCount: 1 });
      else if(VIDEO_EXT.has(ext)) sources.push({ kind: 'video', uri: f, frameCount: this._videoFrames(f) });
    }
    return sources;
  }

  _videoFrames(uri) {
    try {
      const cap = new VideoCapture(uri);
      const n = Math.max(1, Math.round(cap.get(CAP_PROP_FRAME_COUNT)) || 1);
      if(cap.release) cap.release();
      return n;
    } catch(_) {
      return 1;
    }
  }

  // Extract one frame as a fresh Mat. Video captures are cached per-uri so
  // scrubbing in the GUI does not reopen the file each time.
  extractFrame(source, index = 0) {
    if(source.kind === 'image') return capResolution(imread(source.uri));
    const cap = this._cap(source.uri);
    const mat = new Mat();
    try {
      cap.set(CAP_PROP_POS_FRAMES, index);
      cap.read(mat);
    } catch(_) {}
    return capResolution(mat);
  }

  _cap(uri) {
    this._caps = this._caps || new Map();
    if(!this._caps.has(uri)) this._caps.set(uri, new VideoCapture(uri));
    return this._caps.get(uri);
  }
}

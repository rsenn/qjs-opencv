// vectorizer.js
//
// Entry point for the qjs-vectorizer suite. Wires the decoupled pieces together
// and runs the four-stage HighGUI wizard:
//
//   LOAD  -> ASSIGN -> PROCESS -> PROJECT -> (Export) SVG
//
// Usage:
//   qjsm vectorizer.js [options] <image|video|directory> ...
//
// Options:
//   -o, --output FILE   output SVG path           (default: out.svg)
//   -m, --method ID     default method for frames (default: canny)
//   -W, --width  N      output canvas width        (default: 1280)
//   -H, --height N      output canvas height       (default: 720)
//   -h, --help
//
// The pipeline (core/) is pure JS and OpenCV-free; all OpenCV work lives behind
// the loader (cv/loader.js) and the vectorization methods (vector/methods/*).

import * as fs from 'fs';

import { Model } from './core/model.js';
import { Pipeline } from './core/pipeline.js';
import { Composer } from './core/composer.js';
import { defaultRegistry } from './vector/registry.js';
import { Loader } from './cv/loader.js';
import { App } from './gui/app.js';

function parseArgs(argv) {
  const opts = { output: 'out.svg', method: 'canny', width: 1280, height: 720, paths: [] };
  for(let i = 0; i < argv.length; i++) {
    const a = argv[i];
    switch (a) {
      case '-o':
      case '--output':
        opts.output = argv[++i];
        break;
      case '-m':
      case '--method':
        opts.method = argv[++i];
        break;
      case '-W':
      case '--width':
        opts.width = +argv[++i];
        break;
      case '-H':
      case '--height':
        opts.height = +argv[++i];
        break;
      case '-h':
      case '--help':
        opts.help = true;
        break;
      default:
        opts.paths.push(a);
    }
  }
  return opts;
}

function usage() {
  console.log('qjs-vectorizer — extract SVG vector art from images & video');
  console.log('');
  console.log('  qjsm vectorizer.js [-o out.svg] [-m method] [-W w] [-H h] <path>...');
  console.log('');
  console.log('  <path> may be an image, a video, or a directory of either.');
}

function main(...args) {
  const opts = parseArgs(args);
  if(opts.help || opts.paths.length === 0) {
    usage();
    return 0;
  }

  const model = new Model();
  model.canvas.width = opts.width;
  model.canvas.height = opts.height;

  const registry = defaultRegistry();
  const loader = new Loader();
  const composer = new Composer();
  const pipeline = new Pipeline(model, { loader, registry, composer });

  // Stage 1 discovery: turn CLI paths into sources, then attach a cheap
  // first-frame preview (and intrinsic w/h) so the loader grid can render
  // thumbnails without re-reading on every draw.
  const fallbackMethod = registry.has(opts.method) ? opts.method : registry.first().id;
  const sources = pipeline.discover(opts.paths);
  if(!sources.length) {
    console.log('no usable image/video files found');
    return 1;
  }

  for(const s of sources) {
    const src = model.addSource(s.kind, s.uri, s.frameCount);
    try {
      const prev = loader.extractFrame(src, 0);
      src._preview = prev;
      src._w = prev.cols ?? prev.width;
      src._h = prev.rows ?? prev.height;
    } catch(e) {
      console.log('preview failed for', s.uri, String((e && e.message) || e));
    }
  }

  const app = new App({ model, pipeline, registry });

  // When the user hits Export, assign the default method to anything still
  // unassigned (defensive), then write the composed SVG.
  app.onExport = svg => {
    try {
      fs.writeFileSync(opts.output, svg);
      console.log('wrote', opts.output, `(${svg.length} bytes)`);
    } catch(e) {
      console.log('write failed:', String((e && e.message) || e));
    }
  };

  // Give every newly-selected frame a sensible default method, so stage 3 is
  // never empty. Frames are chosen in the stage-1 GUI, so we react to the
  // model's 'frames' event rather than assigning once up front.
  model.on('frames', () => pipeline.autoAssign(fallbackMethod));

  app.run();
  return 0;
}

// qjs passes script args to main(); fall back to scriptArgs for other runtimes.
try {
  const argv = typeof scriptArgs !== 'undefined' ? scriptArgs.slice(1) : [];
  main(...argv);
} catch(e) {
  console.log('fatal:', String((e && e.stack) || e));
}

// Detects the tile grid in a tileset image, cuts it into tiles, drops empty
// and duplicate tiles by image similarity, and repacks the survivors into a
// new (smaller) tileset image plus a JSON sidecar describing the mapping.
//
// Usage: qjsm examples/tileset_grid.js <tileset.png> [outBasename] [--tiles=CxR]
// Options use the --name=value form, see --help.
import * as os from 'os';
import * as std from 'std';
import {
  CommandLineParser,
  Mat,
  Rect,
  CV_32S,
  COLOR_BGR2GRAY,
  COLOR_BGRA2GRAY,
  INTER_AREA,
  IMREAD_UNCHANGED,
  THRESH_BINARY,
  THRESH_BINARY_INV,
  absdiff,
  bitwise_and,
  bitwise_or,
  boundingRect,
  cvtColor,
  imread,
  imwrite,
  meanStdDev,
  countNonZero,
  reduce,
  resize,
  split,
  threshold,
} from 'opencv';

const REDUCE_SUM = 0; // cv::REDUCE_SUM; not exposed as a named constant by this binding

// Minimum/maximum tile size (in pixels) to consider while searching for the
// grid period. Narrow this if you know roughly how big the tiles are.
const MIN_TILE = 8;
const MAX_TILE = 128;

// For images without an alpha channel, a tile counts as "empty" when its
// content is (near-)uniform, i.e. a flat fill with no detail.
const EMPTY_STDDEV_THRESHOLD = 2.0;

// Two tiles count as duplicates when the mean absolute difference of their
// downsampled thumbnails falls below this (0..255 scale).
const DUPLICATE_THRESHOLD = 4.0;
const THUMB_SIZE = 8;

// --recenter: a pixel is padding when it is (near-)white, i.e. every colour
// channel is above WHITE_THRESHOLD, or transparent, i.e. alpha is below
// ALPHA_THRESHOLD.
const WHITE_THRESHOLD = 244;
const ALPHA_THRESHOLD = 16;

function toGray(mat) {
  const gray = new Mat();
  cvtColor(mat, gray, mat.channels() === 4 ? COLOR_BGRA2GRAY : COLOR_BGR2GRAY);
  return gray;
}

// Edge-energy profile: profile[i] is the total absolute difference between
// column i and column i+1 (or row i and row i+1), summed over the whole
// image. Tile boundaries in a uniform grid tend to show up as periodic
// spikes in this profile, even without an explicit separator line, because
// tile art usually changes abruptly at its edges.
function edgeProfile(gray, axis) {
  const { cols, rows } = gray;
  const n = (axis === 'x' ? cols : rows) - 1;
  const a = axis === 'x' ? gray(new Rect(0, 0, cols - 1, rows)) : gray(new Rect(0, 0, cols, rows - 1));
  const b = axis === 'x' ? gray(new Rect(1, 0, cols - 1, rows)) : gray(new Rect(0, 1, cols, rows - 1));

  const diff = new Mat();
  absdiff(a, b, diff);

  const sums = new Mat();
  reduce(diff, sums, axis === 'x' ? 0 : 1, REDUCE_SUM, CV_32S);

  return Array.from(sums.data32S).slice(0, n);
}

// Scores every (step, phase) pair by the average edge energy along the
// grid lines it implies, and returns the best step/offset pair.
// Requires at least this many grid lines on a candidate period before it's
// eligible to win — otherwise a single strong (but non-periodic) edge would
// trivially "win" any step that isolates it as a lone sample.
const MIN_LINES = 3;

function findPeriod(profile, minStep, maxStep) {
  const n = profile.length;
  let best = { step: n, offset: 0, score: -1 };

  for(let step = minStep; step <= Math.min(maxStep, n); step++) {
    if(Math.floor(n / step) < MIN_LINES)
      continue;

    let bestPhaseScore = -1;
    let bestPhase = 0;

    for(let phase = 0; phase < step; phase++) {
      let sum = 0,
        count = 0;

      for(let k = phase; k < n; k += step) {
        sum += profile[k];
        count++;
      }
      if(count < MIN_LINES)
        continue;

      const avg = sum / count;
      if(avg > bestPhaseScore) {
        bestPhaseScore = avg;
        bestPhase = phase;
      }
    }

    if(bestPhaseScore > best.score)
      best = { step, offset: bestPhase, score: bestPhaseScore };
  }

  // profile[k] is the boundary between pixel k and k+1, so the tile that
  // starts *after* that boundary begins at column/row k+1.
  return { size: best.step, offset: (best.offset + 1) % best.step };
}

// Parses 'AxB' / 'A,B' / 'A' (= AxA) into [a, b], or returns undefined.
function parsePair(str) {
  if(!str)
    return undefined;
  const m = /^(\d+)(?:[x,X*](\d+))?$/.exec(str);
  if(!m)
    throw new Error(`invalid value '${str}', expected AxB`);
  return [+m[1], +m[2] || +m[1]];
}

// hints: { tiles: [cols, rows], size: [w, h], offset: [x, y] }
function detectGrid(source, hints = {}) {
  let { tiles, size, offset } = hints;

  if(tiles && !size)
    size = [Math.floor(source.cols / tiles[0]), Math.floor(source.rows / tiles[1])];

  if(size) {
    const [tileWidth, tileHeight] = size;
    const [offsetX, offsetY] = offset ?? [0, 0];
    return { tileWidth, tileHeight, offsetX, offsetY };
  }

  const gray = toGray(source);
  const { size: tileWidth, offset: detX } = findPeriod(edgeProfile(gray, 'x'), MIN_TILE, MAX_TILE);
  const { size: tileHeight, offset: detY } = findPeriod(edgeProfile(gray, 'y'), MIN_TILE, MAX_TILE);
  const [offsetX, offsetY] = offset ?? [detX, detY];

  return { tileWidth, tileHeight, offsetX, offsetY };
}

function stdDev(mat) {
  const mean = new Mat(),
    std = new Mat();
  meanStdDev(mat, mean, std);
  return Math.max(...std.data64F);
}

function meanOf(mat) {
  const mean = new Mat(),
    std = new Mat();
  meanStdDev(mat, mean, std);
  return mean.data64F[0];
}

// With an alpha channel, transparency is the only reliable "empty" signal —
// a flat opaque fill (e.g. a solid-color sky tile) is real content and must
// not be dropped just because it also has near-zero variance. Without
// alpha, fall back to the variance check since there's nothing else to go
// on.
function isEmptyTile(tile) {
  if(tile.channels() === 4) {
    const channels = [];
    split(tile, channels);
    return meanOf(channels[3]) < 1;
  }
  return stdDev(tile) < EMPTY_STDDEV_THRESHOLD;
}

// Crops the padding (white or transparent) off a tile and centres what is
// left on a fresh canvas of the same size: white for opaque images,
// transparent for ones with alpha. Returns the tile itself when it has no
// content or already is centred.
function recenter(tile) {
  const channels = [];
  split(tile, channels);
  const hasAlpha = channels.length === 4;
  const colour = hasAlpha ? channels.slice(0, 3) : channels;

  // content = any colour channel not white
  const content = new Mat();
  threshold(colour[0], content, WHITE_THRESHOLD, 255, THRESH_BINARY_INV);
  for(const ch of colour.slice(1)) {
    const m = new Mat();
    threshold(ch, m, WHITE_THRESHOLD, 255, THRESH_BINARY_INV);
    bitwise_or(content, m, content);
  }
  if(hasAlpha) {
    const opaque = new Mat();
    threshold(channels[3], opaque, ALPHA_THRESHOLD - 1, 255, THRESH_BINARY);
    bitwise_and(content, opaque, content);
  }

  if(countNonZero(content) === 0)
    return tile;

  const box = boundingRect(content);
  const x = Math.floor((tile.cols - box.width) / 2);
  const y = Math.floor((tile.rows - box.height) / 2);
  if(x === box.x && y === box.y)
    return tile;

  const out = new Mat(tile.rows, tile.cols, tile.type(), hasAlpha ? [255, 255, 255, 0] : [255, 255, 255, 255]);
  tile(new Rect(box.x, box.y, box.width, box.height)).copyTo(out(new Rect(x, y, box.width, box.height)));
  return out;
}

function thumbnail(tile) {
  const gray = toGray(tile);
  const thumb = new Mat();
  resize(gray, thumb, { width: THUMB_SIZE, height: THUMB_SIZE }, 0, 0, INTER_AREA);
  return thumb;
}

function similar(a, b) {
  const diff = new Mat();
  absdiff(a, b, diff);
  const mean = new Mat(),
    std = new Mat();
  meanStdDev(diff, mean, std);
  return mean.data64F[0] < DUPLICATE_THRESHOLD;
}

const KEYS = `{ help h usage ? | | print this message }
  { @input | | tileset image }
  { @output | | output basename (default: <input>.packed) }
  { tiles | | tile count as COLSxROWS, e.g. 5x5 }
  { size | | tile size in pixels as WxH, e.g. 32x32 }
  { split | | also save every non-empty tile as its own image, <input>.<row>-<col>.<ext> }
  { outdir | | write all output files into DIR (created if missing) }
  { recenter | | crop white/transparent padding off each tile and centre the content }
  { offset | | grid origin in pixels as X,Y (default: 0,0 with --tiles/--size, else detected) }`;

// Creates dir and any missing parents; throws if one can't be made.
function mkdirs(dir) {
  let path = dir.startsWith('/') ? '/' : '';
  for(const part of dir.split('/').filter(Boolean)) {
    path += part + '/';
    const [st] = os.stat(path);
    if(st && (st.mode & os.S_IFMT) === os.S_IFDIR)
      continue;
    const err = os.mkdir(path, 0o777);
    if(err < 0)
      throw new Error(`cannot create directory '${path}' (errno ${-err})`);
  }
}

function main(...args) {
  const parser = new CommandLineParser(['tileset_grid', ...args], KEYS);
  parser.about('tileset_grid: cut a tileset into tiles, drop empty/duplicate ones, repack\nusage: tileset_grid <tileset.png> [outBasename] [--tiles=CxR] [--size=WxH] [--offset=X,Y] [--split] [--outdir=DIR] [--recenter]');

  if(parser.has('help')) {
    parser.printMessage();
    return 0;
  }

  const filename = parser.get('@input');
  if(!filename) {
    parser.printMessage();
    return 1;
  }

  let hints;
  try {
    hints = { tiles: parsePair(parser.get('tiles')), size: parsePair(parser.get('size')), offset: parsePair(parser.get('offset')) };
  } catch(e) {
    console.log('error:', e.message);
    return 1;
  }

  let outBase = parser.get('@output') || filename.replace(/\.[^.]+$/, '') + '.packed';
  let tileBase = filename;
  const outDir = parser.get('outdir');

  if(outDir) {
    try {
      mkdirs(outDir);
    } catch(e) {
      console.log('error:', e.message);
      return 1;
    }
    const dir = outDir.replace(/\/+$/, '') || '/';
    const inDir = p => `${dir}${dir.endsWith('/') ? '' : '/'}${p.replace(/^.*\//, '')}`;
    outBase = inDir(outBase);
    tileBase = inDir(filename);
  }

  const source = imread(filename, IMREAD_UNCHANGED);
  if(source.empty()) {
    console.log('failed to load', filename);
    return 1;
  }

  const { tileWidth, tileHeight, offsetX, offsetY } = detectGrid(source, hints);
  console.log('detected grid:', { tileWidth, tileHeight, offsetX, offsetY });

  const cols = Math.floor((source.cols - offsetX) / tileWidth);
  const rows = Math.floor((source.rows - offsetY) / tileHeight);
  console.log('grid dimensions:', { cols, rows, tileCount: cols * rows });

  const split = parser.has('split');
  const recenter_ = parser.has('recenter');
  let saved = 0;
  const unique = []; // { thumb, mat, outX, outY }
  const tiles = []; // metadata for every source cell

  for(let gy = 0; gy < rows; gy++) {
    for(let gx = 0; gx < cols; gx++) {
      const rect = new Rect(offsetX + gx * tileWidth, offsetY + gy * tileHeight, tileWidth, tileHeight);
      let tile = source(rect);

      if(isEmptyTile(tile)) {
        tiles.push({ col: gx, row: gy, x: rect.x, y: rect.y, empty: true });
        continue;
      }

      if(recenter_)
        tile = recenter(tile);

      if(split) {
        const m = /^(.*?)(\.[^./]+)?$/.exec(tileBase);
        const pad = n => String(n).padStart(String(Math.max(rows, cols) - 1).length, '0');
        imwrite(`${m[1]}.${pad(gy)}-${pad(gx)}${m[2] ?? '.png'}`, tile);
        saved++;
      }

      const thumb = thumbnail(tile);
      const match = unique.find(u => similar(u.thumb, thumb));

      if(match) {
        tiles.push({ col: gx, row: gy, x: rect.x, y: rect.y, uniqueIndex: match.index });
      } else {
        const index = unique.length;
        unique.push({ index, thumb, mat: tile, x: rect.x, y: rect.y });
        tiles.push({ col: gx, row: gy, x: rect.x, y: rect.y, uniqueIndex: index });
      }
    }
  }

  if(split)
    console.log('saved', saved, 'single tiles as', tileBase.replace(/(\.[^./]+)?$/, '.<row>-<col>$1'));
  console.log('unique tiles:', unique.length, '/', cols * rows);

  const packCols = Math.ceil(Math.sqrt(unique.length || 1));
  const packRows = Math.ceil((unique.length || 1) / packCols);
  const packed = Mat.zeros(packRows * tileHeight, packCols * tileWidth, source.type());

  const packedTiles = unique.map((u, i) => {
    const outX = (i % packCols) * tileWidth;
    const outY = Math.floor(i / packCols) * tileHeight;
    u.mat.copyTo(packed(new Rect(outX, outY, tileWidth, tileHeight)));
    return { index: u.index, outX, outY, sourceX: u.x, sourceY: u.y };
  });

  const outImage = outBase + '.png';
  const outJson = outBase + '.json';
  imwrite(outImage, packed);

  const metadata = {
    source: filename,
    sourceSize: { width: source.cols, height: source.rows },
    tileWidth,
    tileHeight,
    offsetX,
    offsetY,
    grid: { cols, rows },
    packedImage: outImage,
    packedGrid: { cols: packCols, rows: packRows },
    uniqueTiles: packedTiles,
    tiles,
  };

  const f = std.open(outJson, 'w+');
  f.puts(JSON.stringify(metadata, null, 2));
  f.close();

  console.log('wrote', outImage, 'and', outJson);
  return 0;
}

main(...scriptArgs.slice(1));

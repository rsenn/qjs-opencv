// Detects the tile grid in a tileset image, cuts it into tiles, drops empty
// and duplicate tiles by image similarity, and repacks the survivors into a
// new (smaller) tileset image plus a JSON sidecar describing the mapping.
//
// Usage: qjs examples/tileset_grid.js <tileset.png> [outBasename]
import * as std from 'std';
import {
  Mat,
  Rect,
  CV_32S,
  COLOR_BGR2GRAY,
  COLOR_BGRA2GRAY,
  INTER_AREA,
  IMREAD_UNCHANGED,
  absdiff,
  cvtColor,
  imread,
  imwrite,
  meanStdDev,
  reduce,
  resize,
  split,
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

  const flat = axis === 'x' ? sums.array[0] : sums.array.map(row => row[0]);
  return Array.from(flat).slice(0, n);
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

function detectGrid(source) {
  const gray = toGray(source);
  const { size: tileWidth, offset: offsetX } = findPeriod(edgeProfile(gray, 'x'), MIN_TILE, MAX_TILE);
  const { size: tileHeight, offset: offsetY } = findPeriod(edgeProfile(gray, 'y'), MIN_TILE, MAX_TILE);

  return { tileWidth, tileHeight, offsetX, offsetY };
}

function stdDev(mat) {
  const mean = new Mat(),
    std = new Mat();
  meanStdDev(mat, mean, std);
  return Math.max(...std.array.map(row => row[0]));
}

function meanOf(mat) {
  const mean = new Mat(),
    std = new Mat();
  meanStdDev(mat, mean, std);
  return mean.array[0][0];
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
  return mean.array[0][0] < DUPLICATE_THRESHOLD;
}

function main(...args) {
  const filename = args[0];
  if(!filename) {
    console.log('usage: qjs examples/tileset_grid.js <tileset.png> [outBasename]');
    return 1;
  }
  const outBase = args[1] ?? filename.replace(/\.[^.]+$/, '') + '.packed';

  const source = imread(filename, IMREAD_UNCHANGED);
  if(source.empty) {
    console.log('failed to load', filename);
    return 1;
  }

  const { tileWidth, tileHeight, offsetX, offsetY } = detectGrid(source);
  console.log('detected grid:', { tileWidth, tileHeight, offsetX, offsetY });

  const cols = Math.floor((source.cols - offsetX) / tileWidth);
  const rows = Math.floor((source.rows - offsetY) / tileHeight);
  console.log('grid dimensions:', { cols, rows, tileCount: cols * rows });

  const unique = []; // { thumb, mat, outX, outY }
  const tiles = []; // metadata for every source cell

  for(let gy = 0; gy < rows; gy++) {
    for(let gx = 0; gx < cols; gx++) {
      const rect = new Rect(offsetX + gx * tileWidth, offsetY + gy * tileHeight, tileWidth, tileHeight);
      const tile = source(rect);

      if(isEmptyTile(tile)) {
        tiles.push({ col: gx, row: gy, x: rect.x, y: rect.y, empty: true });
        continue;
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

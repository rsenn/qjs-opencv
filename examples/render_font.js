// Renders a TrueType/OpenType font's glyphs into a single 8-bit grayscale
// PNG "font sheet" plus a JSON sidecar describing the cell grid, for use as
// a source atlas by a bitmap font renderer. Cells are sized from the font's
// own metrics (not fixed to 8x8) and can be packed as a grid (power-of-2
// column count, balanced toward square but preferring taller over wider), a
// single horizontal row, or a single vertical column. Codepoints from 0 up
// to the font's highest mapped codepoint (via fc-query) are covered by
// default; ones the font doesn't map get an empty cell in the image and no
// entry in the JSON.
//
// Usage: qjsm examples/render_font.js [options] <font.ttf>
import * as std from 'std';
import * as path from 'path';
import { TextStyle } from '../js/cvHighGUI.js';
import { CommandLineParser, CV_8UC1, INTER_NEAREST, LINE_AA, Mat, Point, absdiff, countNonZero, imwrite, resize } from 'opencv';

const KEYS = `
{help h usage ? |      | print this help and exit}
{@font          |      | path to a TrueType/OpenType font file}
{size s         |      | font pixel size (default: the font's own bitmap size, via fc-query, or 16)}
{start          |      | first character code to render (decimal or 0x.. hex; default: 0)}
{end            |      | last character code to render, exclusive (default: font's highest mapped codepoint + 1)}
{layout l       | grid | glyph layout: grid (square-ish), row (horizontal), or column (vertical)}
{out o          |      | output basename (default: <fontname>@<size>)}
`;

function runCommand(args) {
  const f = std.popen(args.map(a => `'${a.replace(/'/g, `'\\''`)}'`).join(' '), 'r');
  const out = f.readAsString();
  f.close();
  return out;
}

// fc-query wraps FreeType's own font-loading internally (FcFreeTypeQuery),
// so its output matches what FreeType itself reports: `pixelsize` is only
// non-empty for a font with embedded fixed-size bitmap strikes (the actual
// "designed for this size" signal - a scalable outline font has no such
// thing), and `charset` is the real mapped-codepoint ranges, not just
// whatever the format table nominally spans. Returns null silently (empty
// stdout, e.g. fc-query missing or unreadable font) so callers can fall
// back to fixed defaults.
function queryFontMetadata(fontFile) {
  const out = runCommand(['fc-query', '--format=pixelsize=%{pixelsize}\nscalable=%{scalable}\ncharset=%{charset}\n', fontFile]);
  if(!out) return null;

  const pixelSize = /^pixelsize=(\d+(?:\.\d+)?)/m.exec(out);
  const scalable = /^scalable=True/m.test(out);
  const charsetLine = /^charset=(.*)$/ms.exec(out);
  if(!charsetLine) return null;

  const ranges = charsetLine[1]
    .trim()
    .split(/\s+/)
    .filter(Boolean)
    .map(tok => {
      const [lo, hi] = tok.split('-');
      return [parseInt(lo, 16), parseInt(hi ?? lo, 16)];
    });

  if(ranges.length === 0) return null;

  ranges.sort((a, b) => a[0] - b[0]);

  return {
    pixelSize: !scalable && pixelSize ? parseFloat(pixelSize[1]) : null,
    lowestCodepoint: Math.min(...ranges.map(r => r[0])),
    highestCodepoint: Math.max(...ranges.map(r => r[1])),
    ranges,
  };
}

// Binary search over queryFontMetadata()'s sorted [lo, hi] ranges.
function isCovered(ranges, code) {
  let lo = 0,
    hi = ranges.length - 1;

  while(lo <= hi) {
    const mid = (lo + hi) >> 1;
    const [rlo, rhi] = ranges[mid];

    if(code < rlo) hi = mid - 1;
    else if(code > rhi) lo = mid + 1;
    else return true;
  }

  return false;
}

function writeFile(filename, text) {
  const f = std.open(filename, 'w');
  f.puts(text);
  f.close();
}

function parseCode(s) {
  return /^0x/i.test(s) ? parseInt(s, 16) : parseInt(s, 10);
}

// Picks a power-of-2 column count so the sheet stays viewable (never one
// huge horizontal/vertical bar): evaluates the two powers of 2 bracketing
// sqrt(n * cellHeight / cellWidth) (the column count that would make the
// PIXEL dimensions square), scores each by how far its resulting aspect
// ratio deviates from square in log-space (symmetric: 2x-too-wide and
// 2x-too-tall score the same), and on a tie prefers the taller ( width <=
// height) option per the "bigger height than width" preference.
function chooseGridCols(n, cellWidth, cellHeight) {
  const ideal = Math.sqrt((n * cellHeight) / cellWidth);
  const lo = Math.max(1, Math.pow(2, Math.floor(Math.log2(Math.max(ideal, 1)))));
  const hi = lo * 2;

  const aspect = cols => {
    const rows = Math.ceil(n / cols);
    return (cols * cellWidth) / (rows * cellHeight);
  };

  const aLo = aspect(lo),
    aHi = aspect(hi);
  const devLo = Math.abs(Math.log(aLo)),
    devHi = Math.abs(Math.log(aHi));

  if(devLo !== devHi) return devLo < devHi ? lo : hi;
  return aLo <= aHi ? lo : hi;
}

function gridSize(n, layout, cellWidth, cellHeight) {
  if(layout === 'row')
    return { cols: n, rows: 1 };
  if(layout === 'column')
    return { cols: 1, rows: n };
  if(layout === 'grid') {
    const cols = chooseGridCols(n, cellWidth, cellHeight);
    return { cols, rows: Math.ceil(n / cols) };
  }
  throw new Error(`unknown layout '${layout}' (expected grid, row, or column)`);
}

// --- Post-render size sanity check ---
//
// A fixed-size bitmap font rendered at the wrong pixel size doesn't fail
// loudly by itself in every renderer - here it either throws (this
// binding's own FT_Set_Pixel_Sizes assertion for a non-matching size) or
// silently exact-matches, but a bitmap-sourced glyph sheet dragged through
// an unrelated Nx resize afterward, or fed through a renderer that DOES
// rescale bitmaps instead of erroring, produces a recognizable artifact
// instead: either "blockiness" (an exact Nx nearest-neighbor upscale) or
// unexpectedly rich antialiasing (a non-integer mis-scale). Checking for
// both on the actual rendered sheet catches that class of problem even
// when fc-query's metadata alone wouldn't (unavailable, or simply wrong).

// Is `mat` an exact `factor`x nearest-neighbor upscale of a smaller image?
// Decimate by `factor` (nearest samples one pixel per block), re-expand by
// `factor` (nearest replicates it back into a block) - if every factor x
// factor block was already uniform, the round trip reproduces `mat` bit
// for bit.
function blockinessScore(mat, factor) {
  const small = new Mat();
  const back = new Mat();
  resize(mat, small, [Math.round(mat.cols / factor), Math.round(mat.rows / factor)], 0, 0, INTER_NEAREST);
  resize(small, back, [mat.cols, mat.rows], 0, 0, INTER_NEAREST);

  const diff = new Mat();
  absdiff(mat, back, diff);
  return 1 - countNonZero(diff) / (mat.cols * mat.rows);
}

// Counts distinct antialiased gray levels actually used. LINE_AA smooths
// edges of any shape it draws, bitmap-sourced or not, so "some gray
// pixels exist" isn't a signal - what differs is the VARIETY of blend
// ratios. A vector outline's antialiasing computes genuine sub-pixel
// coverage along smooth/diagonal curves and sweeps most of the 8-bit
// range (~150-200 distinct levels, stable across sizes); a bitmap
// strike's already-pixel-snapped stair-step edges only ever produce a
// handful of repeating blend ratios (~20-30 levels), independent of how
// much text is drawn.
function distinctMidgrayLevels(mat) {
  const seen = new Set();
  for(const v of mat.data) if(v > 0 && v < 255) seen.add(v);
  return seen.size;
}

const BITMAP_LEVEL_THRESHOLD = 40;

// Blockiness is checked unconditionally - a real Nx nearest-neighbor
// upscale artifact is equally suspicious in a bitmap or vector-sourced
// render, and a genuinely correct render of either kind essentially never
// triggers it. The level-count/metadata checks below it, though, need a
// ground-truth "this specific font should render crisp" fact to compare
// against - without fc-query's pixelSize confirming this is a bitmap font,
// there's no way to tell a blurred bitmap font from a small, legitimately
// antialiased vector font from a single rendered image alone (verified:
// blur inflates the level count enough to look exactly like a vector
// font's normal antialiasing).
function reportSizeSanity(sheet, fontSize, meta) {
  for(const factor of [2, 3, 4]) {
    if(blockinessScore(sheet, factor) > 0.98) {
      console.log(`size sanity: WARNING - render looks like an exact ${factor}x nearest-neighbor upscale; true size is probably ${fontSize / factor}px, not ${fontSize}px`);
      return;
    }
  }

  if(!meta || !meta.pixelSize) return;

  if(meta.pixelSize !== fontSize) {
    console.log(`size sanity: WARNING - fc-query says this font's native size is ${meta.pixelSize}px, but it was rendered at ${fontSize}px`);
    return;
  }

  const levels = distinctMidgrayLevels(sheet);

  if(levels >= BITMAP_LEVEL_THRESHOLD) {
    console.log(`size sanity: WARNING - bitmap font render has ${levels} distinct antialiased levels (expected < ${BITMAP_LEVEL_THRESHOLD}) - looks mis-scaled despite matching fc-query's native size`);
    return;
  }

  console.log(`size sanity: OK (bitmap font, crisp render at ${fontSize}px, ${levels} distinct antialiased levels)`);
}

function main() {
  // No args array: the constructor then reads the global `scriptArgs`
  // (script path included), matching the argv[0]-is-the-program-name
  // convention the parser expects - passing scriptArgs.slice(1) instead
  // makes it treat the first real option as the program name and silently
  // drop it.
  const parser = new CommandLineParser(KEYS);
  parser.about('Render a TrueType font into a grayscale glyph-sheet PNG.');

  if(!parser.check()) {
    parser.printErrors();
    return 1;
  }

  if(parser.has('help') || !parser.has('@font')) {
    parser.printMessage();
    return parser.has('help') ? 0 : 1;
  }

  const fontFile = parser.get('@font');
  const meta = queryFontMetadata(fontFile);

  const fontSize = parser.get('size') ? parseInt(parser.get('size'), 10) : (meta && meta.pixelSize) || 16;
  const start = parser.get('start') ? parseCode(parser.get('start')) : 0;
  const end = parser.get('end') ? parseCode(parser.get('end')) : (meta ? meta.highestCodepoint + 1 : 127);
  const layout = parser.get('layout');
  const fontName = path.basename(fontFile, path.extname(fontFile));
  const outBase = parser.get('out') || `${fontName}@${fontSize}`;

  if(meta) console.log(`fc-query: pixelSize=${meta.pixelSize ?? '(scalable)'} codepoints=U+${meta.lowestCodepoint.toString(16)}..U+${meta.highestCodepoint.toString(16)}`);

  const style = new TextStyle(fontFile, fontSize);
  const n = end - start;

  // Measure every glyph individually (not just the whole string at once) so
  // proportional-width fonts and glyphs with unusual ascent/descent don't
  // skew a shared cell size derived from an average. Codepoints the font
  // doesn't map (per fc-query's charset - or all of them, if fc-query gave
  // no metadata) are skipped entirely: no measurement, no draw, just an
  // empty cell left at their grid slot.
  let cellWidth = 0,
    ascent = 0,
    descent = 0;

  // See BUGS (opencv-freetype-empty-bbox-garbage-metrics): upstream
  // FreeType2::getTextSize() returns huge-negative garbage height/baseline
  // instead of a sane value whenever the glyph's ink bbox is empty. That's
  // expected/harmless for a lone whitespace glyph (treated as zero-height
  // below), but if EVERY glyph in the range comes back bogus, it means the
  // font+size combination itself is broken (e.g. a bitmap-strike font hit at
  // its own exact native size) - fail clearly instead of crashing later
  // inside Mat.zeros()/putText with an opaque OpenCV assertion.
  const isBogus = (h, d) => !Number.isFinite(h) || !Number.isFinite(d) || h < 0 || h > 10000 || Math.abs(d) > 10000;

  let bogusCount = 0;
  const metrics = [];

  for(let code = start; code < end; code++) {
    if(meta && !isCovered(meta.ranges, code)) continue;

    const ch = String.fromCodePoint(code);
    let glyphDescent;
    const size = style.size(ch, y => (glyphDescent = y));

    if(isBogus(size.height, glyphDescent)) {
      bogusCount++;
      cellWidth = Math.max(cellWidth, size.width);
      metrics.push({ code, ch, glyphAscent: 0, glyphDescent: 0 });
      continue;
    }

    cellWidth = Math.max(cellWidth, size.width);
    ascent = Math.max(ascent, size.height);
    descent = Math.max(descent, glyphDescent);
    metrics.push({ code, ch, glyphAscent: size.height });
  }

  if(metrics.length === 0) throw new Error(`no codepoint in U+${start.toString(16)}..U+${(end - 1).toString(16)} is covered by this font`);

  if(bogusCount === metrics.length)
    throw new Error(
      `getTextSize returned bogus metrics for every glyph at size ${fontSize} - ` +
        `this font+size combination looks unusable (see BUGS: opencv-freetype-empty-bbox-garbage-metrics); try a different --size`,
    );

  const cellHeight = ascent + descent;
  const { cols, rows } = gridSize(n, layout, cellWidth, cellHeight);

  const sheet = Mat.zeros(rows * cellHeight, cols * cellWidth, CV_8UC1);

  // Each glyph's index is its slot in the full [start, end) grid (including
  // the empty slots skipped above), so the sidecar JSON can carry just this
  // one number instead of x/y - a consumer recovers the cell position as
  // `col = index % cols, row = Math.floor(index / cols)` (or `index &
  // (cols - 1)` / `index >>> Math.log2(cols)`, since cols is a power of 2).
  const glyphs = metrics.map(({ code, ch, glyphAscent }) => {
    const index = code - start;
    const col = index % cols;
    const row = Math.floor(index / cols);
    const x = col * cellWidth;
    const y = row * cellHeight;

    // Draw.text's `point` is the glyph box's top-left corner, not its
    // baseline - offset by (ascent - glyphAscent) so every glyph's baseline
    // lands on the same row within its cell regardless of its own ascent.
    //
    // [255] (a 1-element color array) silently draws nothing here - see
    // BUGS (js_color_read-drops-short-arrays) - so pass a plain scalar.
    style.draw(sheet, ch, new Point(x, y + (ascent - glyphAscent)), 255, -1, LINE_AA);

    return { code, char: ch, index };
  });

  const outImage = outBase + '.png';
  const outJson = outBase + '.json';
  imwrite(outImage, sheet);
  reportSizeSanity(sheet, fontSize, meta);

  const metadata = {
    font: fontFile,
    fontSize,
    start,
    end,
    layout,
    cols,
    rows,
    cellWidth,
    cellHeight,
    image: outImage,
    glyphs,
  };

  writeFile(outJson, JSON.stringify(metadata, null, 2));

  console.log('wrote', outImage, 'and', outJson);
  return 0;
}

const code = main();
if(code)
  std.exit(code);

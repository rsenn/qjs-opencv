// Renders a TrueType/OpenType font's glyphs into a single 8-bit grayscale
// PNG "font sheet" plus a JSON sidecar describing the cell grid, for use as
// a source atlas by a bitmap font renderer. Cells are sized from the font's
// own metrics (not fixed to 8x8) and can be packed as a square-ish grid, a
// single horizontal row, or a single vertical column.
//
// Usage: qjsm examples/render_font.js [options] <font.ttf>
import * as std from 'std';
import * as os from 'os';
import * as path from 'path';
import { TextStyle } from '../js/cvHighGUI.js';
import { CommandLineParser, CV_8UC1, LINE_AA, Mat, Point, imwrite } from 'opencv';

const KEYS = `
{help h usage ? |      | print this help and exit}
{@font          |      | path to a TrueType/OpenType font file}
{size s         |      | font pixel size (default: the font's own bitmap size, via fc-query, or 16)}
{start          |      | first character code to render (decimal or 0x.. hex; default: font's lowest mapped codepoint)}
{end            |      | last character code to render, exclusive (default: font's highest mapped codepoint + 1)}
{layout l       | grid | glyph layout: grid (square-ish), row (horizontal), or column (vertical)}
{out o          |      | output basename (default: <fontname>@<size>)}
`;

// fd-based (os.exec/os.pipe/os.read), not std.popen(): same class as
// std.open() (see BUGS: std-file-methods-broken-after-opencv-import), so its
// returned FILE object loses .getline()/.readAsString() once opencv is
// imported too.
function runCommand(args) {
  const [rfd, wfd] = os.pipe();
  const pid = os.exec(args, { stdout: wfd, block: false });
  os.close(wfd);

  const chunks = [];
  const buf = new Uint8Array(4096);
  for(;;) {
    const n = os.read(rfd, buf.buffer, 0, buf.length);
    if(n <= 0) break;
    chunks.push(String.fromCharCode(...buf.subarray(0, n)));
  }
  os.close(rfd);
  os.waitpid(pid, 0);
  return chunks.join('');
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

  return {
    pixelSize: !scalable && pixelSize ? parseFloat(pixelSize[1]) : null,
    lowestCodepoint: Math.min(...ranges.map(r => r[0])),
    highestCodepoint: Math.max(...ranges.map(r => r[1])),
  };
}

// os.open/write (fd-based) instead of std.open(...).puts(...): loading the
// opencv module clobbers std's FILE prototype, see BUGS
// (std-file-methods-broken-after-opencv-import).
function writeFile(filename, text) {
  const bytes = new Uint8Array(text.length);
  for(let i = 0; i < text.length; i++) bytes[i] = text.charCodeAt(i) & 0xff;

  const fd = os.open(filename, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o644);
  os.write(fd, bytes.buffer, 0, bytes.length);
  os.close(fd);
}

function parseCode(s) {
  return /^0x/i.test(s) ? parseInt(s, 16) : parseInt(s, 10);
}

function gridSize(n, layout) {
  if(layout === 'row')
    return { cols: n, rows: 1 };
  if(layout === 'column')
    return { cols: 1, rows: n };
  if(layout === 'grid') {
    const cols = Math.ceil(Math.sqrt(n));
    return { cols, rows: Math.ceil(n / cols) };
  }
  throw new Error(`unknown layout '${layout}' (expected grid, row, or column)`);
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
  const start = parser.get('start') ? parseCode(parser.get('start')) : (meta ? meta.lowestCodepoint : 32);
  const end = parser.get('end') ? parseCode(parser.get('end')) : (meta ? meta.highestCodepoint + 1 : 127);
  const layout = parser.get('layout');
  const fontName = path.basename(fontFile, path.extname(fontFile));
  const outBase = parser.get('out') || `${fontName}@${fontSize}`;

  if(meta) console.log(`fc-query: pixelSize=${meta.pixelSize ?? '(scalable)'} codepoints=U+${meta.lowestCodepoint.toString(16)}..U+${meta.highestCodepoint.toString(16)}`);

  const style = new TextStyle(fontFile, fontSize);
  const codes = [];
  for(let code = start; code < end; code++) codes.push(code);

  // Measure every glyph individually (not just the whole string at once) so
  // proportional-width fonts and glyphs with unusual ascent/descent don't
  // skew a shared cell size derived from an average.
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
  const metrics = codes.map(code => {
    const ch = String.fromCodePoint(code);
    let glyphDescent;
    const size = style.size(ch, y => (glyphDescent = y));

    if(isBogus(size.height, glyphDescent)) {
      bogusCount++;
      cellWidth = Math.max(cellWidth, size.width);
      return { code, ch, glyphAscent: 0, glyphDescent: 0, width: size.width };
    }

    cellWidth = Math.max(cellWidth, size.width);
    ascent = Math.max(ascent, size.height);
    descent = Math.max(descent, glyphDescent);

    return { code, ch, glyphAscent: size.height };
  });

  if(bogusCount === codes.length)
    throw new Error(
      `getTextSize returned bogus metrics for every glyph at size ${fontSize} - ` +
        `this font+size combination looks unusable (see BUGS: opencv-freetype-empty-bbox-garbage-metrics); try a different --size`,
    );

  const cellHeight = ascent + descent;
  const { cols, rows } = gridSize(codes.length, layout);

  const sheet = Mat.zeros(rows * cellHeight, cols * cellWidth, CV_8UC1);

  const glyphs = metrics.map(({ code, ch, glyphAscent }, i) => {
    const col = i % cols;
    const row = Math.floor(i / cols);
    const x = col * cellWidth;
    const y = row * cellHeight;

    // Draw.text's `point` is the glyph box's top-left corner, not its
    // baseline - offset by (ascent - glyphAscent) so every glyph's baseline
    // lands on the same row within its cell regardless of its own ascent.
    //
    // [255] (a 1-element color array) silently draws nothing here - see
    // BUGS (js_color_read-drops-short-arrays) - so pass a plain scalar.
    style.draw(sheet, ch, new Point(x, y + (ascent - glyphAscent)), 255, -1, LINE_AA);

    return { code, char: ch, x, y };
  });

  const outImage = outBase + '.png';
  const outJson = outBase + '.json';
  imwrite(outImage, sheet);

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

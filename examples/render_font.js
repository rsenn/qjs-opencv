// Font tool. Three combinable ways to gather fonts to look at (direct file
// path(s), directories walked recursively, or a filtered fc-list of system
// fonts); a scrollable truecolor terminal browser to eyeball them; and (kept
// from the original single-file tool, unchanged) a PNG+glyph-sheet exporter.
// See TODO.md ("Font tool - render_font.js rebuild") for the staged plan
// this file is being built out against - only Stage 0/1 (gathering +
// terminal browsing) and the pre-existing export path are implemented here;
// detail-view and export-from-the-browser are still TODO.
//
// Usage:
//   qjsm examples/render_font.js <font.ttf> [export options]   - legacy: export one font's PNG+JSON glyph sheet (unchanged)
//   qjsm examples/render_font.js <directory>                   - browse: same as --dirs=<directory>
//   qjsm examples/render_font.js --dirs=<d1,d2,...> [--fclist=<f1,f2,...>] [--fonts=<p1,p2,...>] [--text=<sample>] [--size=<px>]
//                                                                - browse: gather fonts from any combination of the three
//                                                                  sources (a positional <font.ttf>/<directory> is folded in
//                                                                  too) and open the scrollable terminal preview list
import * as std from 'std';
import * as path from 'path';
import { statSync, walkSync } from '../../quickjs/qjs-modules/lib/fs.js';
import { Screen, centeredRect, cursorHide, cursorShow, disableRawMode, enableRawMode, hslToRgb, readKey, setAlternateScreen, setNormalScreen, windowSize, } from '../../quickjs/qjs-modules/lib/terminal.js';
import { TextStyle } from '../js/cvHighGUI.js';
import { CommandLineParser, CV_8UC1, INTER_NEAREST, LINE_AA, Mat, Point, absdiff, countNonZero, imwrite, resize } from 'opencv';

const KEYS = `
{help h usage ? |             | print this help and exit}
{@font          |             | path to a TrueType/OpenType font file (legacy export mode; also folded into browse mode's font list if any of dirs/fclist/fonts is given)}
{size s         |             | font pixel size - export mode: default is the font's own bitmap size via fc-query, or 16; browse mode: default 14}
{start          |             | export mode: first character code to render (decimal or 0x.. hex; default: 0)}
{end            |             | export mode: last character code to render, exclusive (default: font's highest mapped codepoint + 1)}
{layout l       | grid        | export mode: glyph layout: grid (square-ish), row (horizontal), or column (vertical)}
{out o          |             | export mode: output basename (default: <fontname>@<size>)}
{dirs           |             | browse mode: comma-separated directories to walk recursively for font files}
{fclist         |             | browse mode: comma-separated fc-list family-name filter substrings (matches system fonts)}
{fonts          |             | browse mode: comma-separated additional font file paths}
{text t         | ABC#!$@123  | browse mode: sample string to render (can also be typed live with the 't' key)}
`;

function runCommand(args) {
  // std.popen runs via a shell, and stderr defaults to this process's own
  // (the raw-mode, alt-screen terminal) - any subprocess warning/traceback
  // would otherwise print straight through underneath the UI instead of
  // being caught by the try/catch call sites here that expect a clean
  // failure (empty stdout). Redirect it away instead of inheriting it.
  const cmd = args.map(a => `'${a.replace(/'/g, `'\\''`)}'`).join(' ') + ' 2>/dev/null';
  const f = std.popen(cmd, 'r');
  const out = f.readAsString();
  f.close();
  return out;
}

// ============================================================================
// SECTION: info-gathering
// ============================================================================

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

// Is `mat` an exact `factor`x nearest-neighbor upscale of a smaller image?
// Decimate by `factor` (nearest samples one pixel per block), re-expand by
// `factor` (nearest replicates it back into a block) - if every factor x
// factor block was already uniform, the round trip reproduces `mat` bit
// for bit. Used by the export-view size-sanity check below, and by
// findCrispSize()'s candidate-size search further down.
function blockinessScore(mat, factor) {
  const small = new Mat();
  const back = new Mat();
  resize(mat, small, [Math.round(mat.cols / factor), Math.round(mat.rows / factor)], 0, 0, INTER_NEAREST);
  resize(small, back, [mat.cols, mat.rows], 0, 0, INTER_NEAREST);

  const diff = new Mat();
  absdiff(mat, back, diff);

  // Normalized against ink pixels, not total pixels: a sparse sheet (most
  // codepoints in range uncovered, per render_font.js's own coverage
  // feature - the common case for a wide default range) is mostly blank
  // background, which trivially round-trips through any decimate/re-expand
  // regardless of whether the actual glyphs are blocky - normalizing by the
  // total pixel count let that blank majority mask real (non-)blockiness in
  // the glyphs themselves and misfire on a correctly-sized, correctly sparse
  // render (found by testing this exact function against real fonts).
  const ink = countNonZero(mat);
  if(ink === 0) return 1;
  return 1 - countNonZero(diff) / ink;
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

// Runs a short Python/fontTools one-liner to pull program-level facts
// `fc-query` can't give us: units-per-em, glyph count, outline format
// (TrueType `glyf` vs CFF), color-glyph tables, whether the TrueType
// hinting bytecode program (`fpgm`/`prep`) is actually present and
// non-empty, variable-font axes, and - the useful part for "why is this
// pixel font rendering antialiased" - a best-effort "pixel grid unit" for
// scalable fonts: the GCD of all glyph outline point coordinates sampled
// across up to 60 glyphs. A font whose design deliberately snaps every
// point to an NxN pixel grid (in font units) will usually have a large,
// consistent GCD; dividing unitsPerEm by that GCD's GCD-with-unitsPerEm
// gives the smallest pixel size at which one grid cell lands on exactly
// one integer physical pixel - candidate sizes are multiples of that.
//
// This deliberately replaces the "write a native FreeType/HarfBuzz C++
// binding" idea from the original plan (see TODO.md) - fontTools (already
// installed here) exposes the exact same program-level facts through
// Python, shelled out to exactly like fc-query already is; nothing in
// this list actually needs FreeType's own internals; a native binding
// would only add rendering-path-level detail this tool doesn't use.
// Returns null on any failure (fontTools not installed, corrupt font,
// python3 missing) - the caller falls back to the empirical search alone.
function queryFontProgramInfo(fontFile) {
  // One Python statement per array entry, joined with real '\n's - NOT a
  // single template-literal string. A flattened one-liner here is a
  // guaranteed Python SyntaxError (indentation-sensitive `try`/`for`/`if`
  // blocks can't be space-joined onto one physical line), and since
  // runCommand() doesn't redirect stderr, that traceback used to print
  // straight through to the raw terminal underneath the alt-screen UI.
  const script = [
    'import sys, json',
    'from math import gcd',
    'from functools import reduce',
    'try:',
    '    from fontTools.ttLib import TTFont',
    'except Exception as e:',
    '    print(json.dumps({"error": "fontTools not available: %s" % e}))',
    '    sys.exit(0)',
    'try:',
    '    f = TTFont(sys.argv[1], lazy=True, fontNumber=0)',
    '    info = {}',
    '    info["unitsPerEm"] = f["head"].unitsPerEm if "head" in f else None',
    '    info["numGlyphs"] = f["maxp"].numGlyphs if "maxp" in f else None',
    '    info["outline"] = "CFF" if "CFF " in f else ("glyf" if "glyf" in f else "bitmap-only")',
    '    info["colorTables"] = [t for t in ("COLR", "CPAL", "sbix", "CBDT", "CBLC") if t in f]',
    '    info["embeddedBitmaps"] = ("EBDT" in f) or ("CBDT" in f)',
    '    info["variableAxes"] = [a.axisTag for a in f["fvar"].axes] if "fvar" in f else []',
    '    hasHinting = False',
    '    try:',
    '        if "fpgm" in f and len(f["fpgm"].program.getBytecode()) > 0:',
    '            hasHinting = True',
    '        elif "prep" in f and len(f["prep"].program.getBytecode()) > 0:',
    '            hasHinting = True',
    '    except Exception:',
    '        pass',
    '    info["hasHinting"] = hasHinting',
    '    gridUnit = 0',
    '    if "glyf" in f:',
    '        coords = []',
    '        count = 0',
    '        for name in f.getGlyphOrder():',
    '            if count >= 60:',
    '                break',
    '            g = f["glyf"][name]',
    '            if g.isComposite() or not hasattr(g, "coordinates"):',
    '                continue',
    '            for (x, y) in g.coordinates:',
    '                if x:',
    '                    coords.append(abs(int(x)))',
    '                if y:',
    '                    coords.append(abs(int(y)))',
    '            count += 1',
    '        if coords:',
    '            gridUnit = reduce(gcd, coords)',
    '    info["gridUnit"] = gridUnit',
    '    info["gridSizeBase"] = (info["unitsPerEm"] // gcd(info["unitsPerEm"], gridUnit)) if (gridUnit and info["unitsPerEm"]) else None',
    '    print(json.dumps(info))',
    'except Exception as e:',
    '    print(json.dumps({"error": str(e)}))',
  ].join('\n');
  const out = runCommand(['python3', '-c', script, fontFile]);
  if(!out) return null;

  try {
    const info = JSON.parse(out.trim().split('\n').pop());
    return info.error ? null : info;
  } catch(e) {
    return null;
  }
}

// Bounded search (see the comment on queryFontProgramInfo above for the
// "gridSizeBase" hint) for a pixel size at which this font renders
// bilevel/crisp rather than antialiased - the actual "at what size does
// THIS font look pixel-perfect" question this session was asked to solve.
//
// Deliberately NOT exhaustive: an embedded bitmap-strike font is already
// known-crisp at its one native size (no search needed - same fc-query
// signal the exporter already trusts). A scalable font instead gets a
// short, bounded candidate list - the grid hint from queryFontProgramInfo
// (when it lands in a plausible pixel-size range) plus a fixed set of
// common pixel-font sizes - each rendered once and measured with the same
// distinctMidgrayLevels() heuristic the exporter's size-sanity check
// already relies on. At most ~10 renders total, only ever run for the one
// font currently open in the detail view - never for a whole directory,
// which is what actually keeps this from "searching forever".
function findCrispSize(fontFile, meta, programInfo) {
  if(meta && meta.pixelSize) return { method: 'bitmap-strike', tried: [], best: { size: meta.pixelSize, levels: 0, crisp: true } };

  const candidates = new Set([8, 10, 12, 14, 16, 20, 24, 32]);
  const gridHint = programInfo && programInfo.gridSizeBase;

  if(gridHint && gridHint >= 4 && gridHint <= 64) {
    candidates.add(gridHint);
    candidates.add(gridHint * 2);
    candidates.add(gridHint * 3);
  }

  const sizes = [...candidates].sort((a, b) => a - b).slice(0, 10);
  const tried = [];
  let best = null;

  for(const size of sizes) {
    let probe;

    try {
      probe = renderGlyphMat(new TextStyle(fontFile, size), size, 'Aa0Gg8');
    } catch(e) {
      continue;
    }

    if(!probe) continue;

    const levels = distinctMidgrayLevels(probe.mat);
    const crisp = levels < BITMAP_LEVEL_THRESHOLD;

    tried.push({ size, levels, crisp });
    if(!best || levels < best.levels) best = { size, levels, crisp };
  }

  return { method: 'empirical', gridHint: gridHint || null, tried, best };
}

// A curated (not exhaustive) set of Unicode blocks worth calling out for
// this tool's actual purpose - LCD text/UI use - rather than a full
// Unicode block database: Latin for ordinary text, a few common script
// blocks, then the ranges the original brief specifically cared about
// (box-drawing/block-elements for UI chrome, braille for dense
// sub-pixel-style rendering, PUA/Powerline for icon fonts, emoji).
const NAMED_BLOCKS = [
  ['Basic Latin', 0x20, 0x7e],
  ['Latin-1 Supplement', 0xa0, 0xff],
  ['Latin Extended-A/B', 0x100, 0x24f],
  ['Greek', 0x370, 0x3ff],
  ['Cyrillic', 0x400, 0x4ff],
  ['General Punctuation', 0x2000, 0x206f],
  ['Box Drawing', 0x2500, 0x257f],
  ['Block Elements', 0x2580, 0x259f],
  ['Braille Patterns', 0x2800, 0x28ff],
  ['Private Use Area', 0xe000, 0xf8ff],
  ['Powerline/Nerd Symbols', 0xe0a0, 0xe0d4],
  ['Emoji (misc symbols+pictographs)', 0x1f300, 0x1faff],
];

// Summarizes fc-query's coverage ranges (already parsed by
// queryFontMetadata) against NAMED_BLOCKS, plus the overall span/count.
function summarizeCoverage(meta) {
  if(!meta) return null;

  const blocks = NAMED_BLOCKS.map(([name, lo, hi]) => {
    let covered = 0;
    for(let c = lo; c <= hi; c++) if(isCovered(meta.ranges, c)) covered++;
    return { name, lo, hi, covered, total: hi - lo + 1 };
  }).filter(b => b.covered > 0);

  const totalCovered = meta.ranges.reduce((sum, [lo, hi]) => sum + (hi - lo + 1), 0);

  return { lowest: meta.lowestCodepoint, highest: meta.highestCodepoint, totalCovered, rangeCount: meta.ranges.length, blocks };
}

// Flattens fc-query's coverage ranges into one dense list of actual
// covered codepoints, index-packed like the PNG/JSON exporter's grid
// (skipping the uncovered gaps entirely) rather than laid out by raw
// codepoint value - most fonts here cover a sparse handful of ranges
// (Latin + punctuation + maybe one symbol block far away in the
// codepoint space), and a dense-by-value grid would be mostly wasted
// blank rows. Capped: a font with a genuinely huge coverage (e.g. a CJK
// font) would otherwise build an enormous array just to page through a
// glyph map interactively.
const MAX_GLYPHMAP_CODEPOINTS = 8192;

function flattenCoveredCodepoints(meta, cap = MAX_GLYPHMAP_CODEPOINTS) {
  if(!meta) return [];

  const out = [];

  for(const [lo, hi] of meta.ranges) {
    for(let c = lo; c <= hi; c++) {
      out.push(c);
      if(out.length >= cap) return out;
    }
  }

  return out;
}

// If `text` has a codepoint this font doesn't cover, a preview render of
// it would show FreeType's own .notdef "tofu" box for that character
// instead of a real glyph - misleading in a list of previews (looks like
// a font problem, not a "wrong sample text for this font" problem).
// Falls back to a sample built from the font's OWN covered codepoints
// (preferring the printable-ASCII-ish range, since that's what a sample
// is for) when `text` isn't fully coverable; returns `text` unchanged
// when it is, or when there's no coverage metadata to check against.
function pickSampleText(text, meta, length = 10) {
  if(!meta) return text;

  const fullyCovered = [...text].every(ch => isCovered(meta.ranges, ch.codePointAt(0)));
  if(fullyCovered) return text;

  const codepoints = flattenCoveredCodepoints(meta, 4096);
  const printable = codepoints.filter(c => c >= 0x21 && c <= 0x7e);
  const pool = printable.length > 0 ? printable : codepoints;

  return pool
    .slice(0, length)
    .map(c => String.fromCodePoint(c))
    .join('');
}

function expandHome(p) {
  if(p === '~' || p.startsWith('~/')) {
    const home = std.getenv('HOME');
    if(home) return home + p.slice(1);
  }
  return p;
}

function splitList(s) {
  return s
    .split(',')
    .map(x => x.trim())
    .filter(Boolean);
}

// Gathers font files from the three combinable sources (direct paths,
// recursive directory walk, filtered fc-list) into one deduplicated list of
// { path, source } descriptors.
class FontCatalog {
  static EXTENSIONS = ['.ttf', '.otf', '.ttc', '.otc', '.pcf', '.pcf.gz', '.bdf'];

  static isFontFile(name) {
    const lower = name.toLowerCase();
    return FontCatalog.EXTENSIONS.some(ext => lower.endsWith(ext));
  }

  static fromFiles(paths) {
    return paths.map(p => ({ path: p, source: 'file' }));
  }

  static fromDirs(dirs) {
    const out = [];

    for(const dir of dirs) {
      // onError: a real font stash tends to have unreadable subdirectories
      // (permissions, broken symlinks) somewhere in it - one bad subdir
      // shouldn't abort gathering every other font found so far.
      for(const entry of walkSync(expandHome(dir), {
        includeDirs: false,
        filter: e => FontCatalog.isFontFile(e.name),
        onError: (e, d) => console.log(`warning: skipping unreadable directory '${d}': ${e.message}`),
      }))
        out.push({ path: entry.path, source: 'dir' });
    }

    return out;
  }

  // `filters` are required (non-empty) - fc-list unfiltered is 2000+ system
  // fonts, too many to usefully browse, so an empty filter list yields
  // nothing rather than silently dumping the whole system font catalog.
  static fromFcList(filters) {
    if(filters.length === 0) return [];

    const needles = filters.map(f => f.toLowerCase());
    const out = runCommand(['fc-list', '--format=%{file}\t%{family}\n']);
    const result = [];

    for(const line of out.split('\n')) {
      if(!line) continue;

      const tab = line.indexOf('\t');
      const file = tab === -1 ? line : line.slice(0, tab);
      const family = (tab === -1 ? '' : line.slice(tab + 1)).toLowerCase();

      if(file && needles.some(n => family.includes(n))) result.push({ path: file, source: 'fclist', family: line.slice(tab + 1) });
    }

    return result;
  }

  // Combines any subset of the three sources, deduplicated by path (later
  // sources win the `source`/`family` tag on a duplicate).
  static gather({ files = [], dirs = [], fclistFilters = [] } = {}) {
    const byPath = new Map();

    for(const entry of [...FontCatalog.fromDirs(dirs), ...FontCatalog.fromFcList(fclistFilters), ...FontCatalog.fromFiles(files)]) byPath.set(entry.path, entry);

    return [...byPath.values()].sort((a, b) => a.path.localeCompare(b.path));
  }
}

// ============================================================================
// SECTION: demoing (terminal truecolor rendering)
// ============================================================================

const ESC = '\x1b';

// Truncates a (possibly ANSI-colored) line to at most `maxCols` VISIBLE
// cells, dropping CSI escape sequences (`ESC [ ... letter`) from the count
// entirely and always appending a fresh SGR reset so no color state can
// bleed past the cut point. Used to guarantee a rendered line never
// reaches the terminal's last column - some terminals (observed: xfce4
// under GNU screen, TERM=screen-256color; not the same xfce4-terminal
// outside screen) don't defer autowrap the way a plain xterm does, so a
// character actually written to the last column immediately wraps and
// shifts every following row down by one, wrecking a multi-row render
// that otherwise relies on the cursor staying put between writes. Content
// is always measured/rendered at full width first (a big font's sample
// can be far wider than the terminal) and only clipped here, at the very
// last step before it reaches the terminal.
function clipAnsiLine(line, maxCols) {
  if(maxCols <= 0) return '';

  let out = '',
    visible = 0,
    i = 0;

  while(i < line.length && visible < maxCols) {
    if(line[i] === ESC) {
      const start = i++;
      if(line[i] === '[') {
        i++;
        while(i < line.length && !/[a-zA-Z]/.test(line[i])) i++;
        i++;
      }
      out += line.slice(start, i);
    } else {
      out += line[i++];
      visible++;
    }
  }

  return out + `${ESC}[0m`;
}

// Two vertically-stacked pixel rows become one terminal cell via U+2584
// LOWER HALF BLOCK: its background fill is the cell's top half, its
// foreground glyph fill is the bottom half - so a background truecolor set
// to the top pixel's gray level and a foreground truecolor set to the
// bottom pixel's gives ~2x the vertical resolution a plain colored space
// character could. Renders rows [yStart, yEnd) of `mat` only. `colorFn`
// maps a pixel's 0-255 intensity to an [r,g,b] triple - defaults to plain
// grayscale (v => [v,v,v]); colorWheelPalette() below is the other
// caller, for the glyph map's toggleable color mode.
function matToHalfBlocks(mat, yStart, yEnd, colorFn = v => [v, v, v]) {
  const { cols } = mat;
  const data = mat.data;
  const px = (x, y) => (y < yEnd ? data[y * cols + x] : 0);
  const lines = [];

  for(let y = yStart; y < yEnd; y += 2) {
    let line = '',
      lastTop = -1,
      lastBot = -1;

    for(let x = 0; x < cols; x++) {
      const top = px(x, y);
      const bot = px(x, y + 1);

      if(top !== lastTop) {
        const [r, g, b] = colorFn(top);
        line += `${ESC}[48;2;${r};${g};${b}m`;
        lastTop = top;
      }
      if(bot !== lastBot) {
        const [r, g, b] = colorFn(bot);
        line += `${ESC}[38;2;${r};${g};${b}m`;
        lastBot = bot;
      }
      line += '▄';
    }

    line += `${ESC}[0m`;
    lines.push(line);
  }

  return lines;
}

// The glyph map's toggleable color mode: instead of plain gray, maps
// intensity to a single hue's arc of the color wheel - dark/desaturated
// for background-level pixels, vivid/bright for ink-level ones - so the
// antialiasing gradient a glyph is built from becomes a real color
// gradient without losing the light/dark contrast its shape depends on.
// `hue` is degrees on the wheel; see FontListBrowser's key handling for
// how a fresh hue gets picked each time the mode is turned on.
function colorWheelPalette(hue) {
  return v => hslToRgb(hue, 0.15 + (v / 255) * 0.55, 0.1 + (v / 255) * 0.55);
}

// The row range actually containing ink, or null if the Mat is blank.
function inkRowBounds(mat) {
  const { cols, rows, data } = mat;
  let minY = -1,
    maxY = -1;

  for(let y = 0; y < rows; y++) {
    let hasInk = false;
    for(let x = 0; x < cols; x++)
      if(data[y * cols + x] > 0) {
        hasInk = true;
        break;
      }

    if(hasInk) {
      if(minY === -1) minY = y;
      maxY = y;
    }
  }

  return minY === -1 ? null : [minY, maxY];
}

// Renders `text` with `style` (already constructed at `size`) into a Mat
// cropped tightly to its actual ink, or null if nothing renders. Shared by
// TerminalGlyphRenderer below (converts the Mat to terminal half-blocks)
// and findCrispSize() in the info-gathering section above (measures its
// antialiasing level instead).
//
// See BUGS (freetype-puttext-y-ignores-gettextsize-ascent): for a scalable
// font, `Draw.text()`'s `point.y` is NOT "top of the ink bbox implied by
// `TextStyle.size()`'s reported ascent" - empirically the glyph renders
// somewhere in [0.5, 1.0] * fontSize *below* whatever `point.y` is given,
// regardless of `size()`'s ascent/descent numbers. Rather than trust that
// offset (which the legacy exporter below does, and loses the top row of
// its grid as a result - see the BUGS entry), draw into a canvas tall
// enough to contain that worst case at point.y=0, then crop to the actual
// ink afterward. `size()`'s `.width` is unaffected by this bug and is used
// normally for glyph advance.
function renderGlyphMat(style, size, text) {
  const widths = [...text].map(ch => style.size(ch).width);
  const width = widths.reduce((a, b) => a + b, 0);

  if(width <= 0) return null;

  const canvasHeight = size * 2 + 4;
  const mat = Mat.zeros(canvasHeight, width, CV_8UC1);
  let x = 0;

  for(let i = 0; i < text.length; i++) {
    // See BUGS (js_color_read-drops-short-arrays): a 1-element color
    // array silently draws nothing, so pass a plain scalar.
    style.draw(mat, text[i], new Point(x, 0), 255, -1, LINE_AA);
    x += widths[i];
  }

  const bounds = inkRowBounds(mat);
  if(!bounds) return null;

  return { mat, top: bounds[0], bottom: bounds[1] };
}

// Renders a sample string with a font at a given pixel size into truecolor
// half-block terminal lines.
class TerminalGlyphRenderer {
  constructor(fontFile, size) {
    this.style = new TextStyle(fontFile, size);
    this.size = size;
  }

  renderLines(text) {
    const probe = renderGlyphMat(this.style, this.size, text);
    if(!probe) return ['(no renderable glyphs at this size)'];

    return matToHalfBlocks(probe.mat, probe.top, probe.bottom + 1);
  }
}

// Codepoints per glyph-map row, like a classic hex charmap (row = 16
// consecutive entries of the covered-codepoint list from
// flattenCoveredCodepoints - index-packed, not raw codepoint value; see
// its comment). Deliberately not tied to terminal width - a 16-wide row
// at any reasonable cell width is normally wider than one terminal, which
// is exactly why the glyph map needs left/right panning, not just
// wrapping.
const GLYPHMAP_COLS = 16;

// A uniform cell width (in font pixels = terminal columns) for the whole
// map, so glyph columns actually line up - sampled from (at most) the
// first 200 covered codepoints rather than the whole list, since that
// list can be up to MAX_GLYPHMAP_CODEPOINTS entries.
function computeGlyphMapCellWidth(style, codepoints, sampleCount = 200) {
  let width = 1;

  for(let i = 0; i < Math.min(sampleCount, codepoints.length); i++) {
    try {
      const w = style.size(String.fromCodePoint(codepoints[i])).width;
      if(w > width) width = w;
    } catch(e) {}
  }

  return width;
}

// Renders one glyph-map row (up to `visibleCols` glyphs starting at
// `codepoints[startIndex]`, each left-aligned in its own `cellWidth`-wide
// slot) as truecolor half-block terminal lines. Unlike
// TerminalGlyphRenderer.renderLines()/renderGlyphMat(), this does NOT
// autocrop to ink - every row needs the same fixed vertical window so
// cells actually align from row to row; per the calibration in BUGS
// (freetype-puttext-y-ignores-gettextsize-ascent), ink for a glyph drawn
// at point.y=0 never lands above row 0 or at/after row `size`, so
// [0, size) reliably contains it uniformly across every glyph tried.
function renderGlyphMapRow(style, size, cellWidth, codepoints, startIndex, visibleCols, colorFn) {
  const canvasHeight = size * 2 + 4;
  const mat = Mat.zeros(canvasHeight, cellWidth * visibleCols, CV_8UC1);

  for(let c = 0; c < visibleCols; c++) {
    const cp = codepoints[startIndex + c];
    if(cp === undefined) continue;

    try {
      style.draw(mat, String.fromCodePoint(cp), new Point(c * cellWidth, 0), 255, -1, LINE_AA);
    } catch(e) {}
  }

  return matToHalfBlocks(mat, 0, size, colorFn);
}

// ============================================================================
// SECTION: detail-view
// ============================================================================
// Scoped-down Stage 3 (see TODO.md): NOT a generic metadata dump - only
// what's needed to judge whether/at-what-size a font renders crisp, plus a
// character-coverage overview, per the actual ask this was built for
// ("some fonts in a directory are displayed at the wrong size and are
// thus antialiased - find out why and what render_font.js can determine
// about it"). Pulls together queryFontMetadata/queryFontProgramInfo/
// findCrispSize/summarizeCoverage from info-gathering into one struct,
// and a pure line-formatting function the browser section below draws.

// Panel color scheme: two pastel hues, each a light and a dark shade.
// PANEL_HUE is the panel's structural color (light for the border, dark
// for the background fill it occludes whatever's behind it with);
// ACCENT_HUE highlights specific values inline within the panel's body
// text (a recommended size, a coverage percentage, a "yes" flag) via
// panelHighlight() below, kept visually distinct from the panel chrome.
const PANEL_HUE = 200; // cool teal-blue
const ACCENT_HUE = 28; // warm coral

const PANEL_BORDER_RGB = hslToRgb(PANEL_HUE, 0.45, 0.78);
const PANEL_BG_RGB = hslToRgb(PANEL_HUE, 0.35, 0.16);
const PANEL_TEXT_RGB = hslToRgb(PANEL_HUE, 0.12, 0.92);
const PANEL_ACCENT_RGB = hslToRgb(ACCENT_HUE, 0.6, 0.78);

function ansiRgb(kind, [r, g, b]) {
  return `${ESC}[${kind};2;${r};${g};${b}m`;
}

// Colors `text` with the panel's accent hue, then returns to the panel's
// OWN base text/background colors (never a blanket SGR reset, which
// would also wipe the panel's background tint) - lets a value be
// highlighted inline within an otherwise normally-colored panel line.
function panelHighlight(text) {
  return `${ansiRgb(38, PANEL_ACCENT_RGB)}${text}${ansiRgb(38, PANEL_TEXT_RGB)}${ansiRgb(48, PANEL_BG_RGB)}`;
}

// The glyph map's coarse size-cycle list: every size findCrispSize()
// actually tried (so cycling through it always lands on a size that's
// been measured, crisp or not - see the tried/levels readout), plus the
// "obviously good" 2x/4x of the best-guess (or fallback) size - an
// integer multiple of an already-crisp pixel size stays crisp (just
// blockier), the same logic the exporter's own blockiness check relies
// on elsewhere in this file. Sorted ascending, deduplicated, floored at
// 2px.
function goodSizesFor(crisp, fallbackSize) {
  const sizes = new Set();
  const base = crisp && crisp.best ? crisp.best.size : fallbackSize;

  if(crisp) for(const t of crisp.tried) sizes.add(t.size);
  sizes.add(base);
  sizes.add(base * 2);
  sizes.add(base * 4);

  return [...sizes].filter(s => s >= 2).sort((a, b) => a - b);
}

function gatherDetailInfo(fontFile) {
  const meta = queryFontMetadata(fontFile);
  const programInfo = queryFontProgramInfo(fontFile);
  const crisp = findCrispSize(fontFile, meta, programInfo);
  const coverage = summarizeCoverage(meta);

  return { fontFile, meta, programInfo, crisp, coverage };
}

// Content uses Unicode line-art/pictograms (┄ dividers, ▸ bullets, ✓/✗/⚠
// markers, · separators) in place of plain "--" headers, "->" arrows, and
// parenthetical asides, per direct instruction.
function formatCrispSizeLines(crisp) {
  const lines = ['◆ Crisp Render Size', '┄'.repeat(20)];

  if(crisp.method === 'bitmap-strike') {
    lines.push(`✓ embedded bitmap strike, always crisp at ${panelHighlight(crisp.best.size + 'px')}`);
    return lines;
  }

  lines.push(crisp.gridHint ? `▸ glyph-outline grid hint · ${panelHighlight(crisp.gridHint + 'px')}` : '▸ no usable glyph-outline grid hint');

  if(crisp.tried.length === 0) {
    lines.push('⚠ no candidate sizes could be rendered · font failed to load');
    return lines;
  }

  lines.push('▸ tried ' + crisp.tried.map(t => `${t.size}px${t.crisp ? '✓' : '·' + t.levels}`).join('  '));

  if(crisp.best && crisp.best.crisp) lines.push(`✓ best guess ${panelHighlight(crisp.best.size + 'px')} looks crisp · ${crisp.best.levels} levels → try --size=${crisp.best.size}`);
  else lines.push(`⚠ no crisp size found among sizes tried → likely a genuinely antialiased/vector font`);

  return lines;
}

function formatCoverageLines(coverage) {
  if(!coverage) return ['▦ Character Coverage', '┄'.repeat(20), '⚠ fc-query gave no charset for this font'];

  const lines = [
    '▦ Character Coverage',
    '┄'.repeat(20),
    `▸ U+${coverage.lowest.toString(16)}..U+${coverage.highest.toString(16)} · ${panelHighlight(coverage.totalCovered + ' codepoints')} across ${coverage.rangeCount} ranges`,
  ];

  for(const b of coverage.blocks) {
    const pct = Math.round((100 * b.covered) / b.total);
    lines.push(`  ▸ ${b.name.padEnd(26)} ${b.covered}/${b.total} ${panelHighlight(pct + '%')}`);
  }

  return lines;
}

function renderDetailLines(info) {
  const { fontFile, meta, programInfo, crisp, coverage } = info;

  const lines = [
    panelHighlight(path.basename(fontFile)),
    `▸ file ${fontFile}`,
    meta ? `▸ format ${meta.pixelSize ? `fixed bitmap strike, native ${meta.pixelSize}px` : 'scalable outline'}` : '⚠ format unknown · fc-query gave no metadata',
    programInfo
      ? `▸ outline ${programInfo.outline} · em ${programInfo.unitsPerEm} · glyphs ${programInfo.numGlyphs} · hinting ${programInfo.hasHinting ? '✓' : '✗'}`
      : '⚠ outline/em/glyphs/hinting unknown · fontTools unavailable or failed',
  ];

  if(programInfo)
    lines.push(
      `▸ color tables ${programInfo.colorTables.length ? programInfo.colorTables.join(',') : '✗ none'} · variable axes ${programInfo.variableAxes.length ? programInfo.variableAxes.join(',') : '✗ none'}`,
    );

  lines.push('', ...formatCrispSizeLines(crisp), '', ...formatCoverageLines(coverage));

  return lines;
}

// ============================================================================
// SECTION: browsing
// ============================================================================

// Scrollable, up/down-navigable terminal list of fonts, each entry showing
// its path and a truecolor half-block rendering of the current sample
// text. Space toggles a per-entry mark (kept for a future Stage 4 "export
// the marked fonts" step - not wired to anything yet). 't' opens a small
// in-place line editor to change the sample text live.
class FontListBrowser {
  #fonts;
  #sampleText;
  #previewSize;
  #selected = 0;
  #scrollTop = 0;
  #marked = new Set();
  #cache = new Map();
  #detailCache = new Map();
  #glyphMapCache = new Map();
  // Golden-angle (~137.508deg) running hue offset for the glyph map's
  // color-mode toggle: each time color mode is turned on it takes the
  // next step around the wheel, so repeatedly toggling it across a
  // session (even across different fonts) walks a well-spread sequence
  // of hues instead of repeating or clustering - the "different palette
  // every time" this was asked for.
  #nextHue = 0;
  #status = '';

  constructor(fonts, { sampleText = 'ABC#!$@123', previewSize = 14 } = {}) {
    this.#fonts = fonts;
    this.#sampleText = sampleText;
    this.#previewSize = previewSize;
  }

  // Detects (once per font, cached) the size/sample this font actually
  // previews best at, via the same findCrispSize()/pickSampleText()
  // machinery the detail view uses - "reading" a font into the list now
  // means finding out how it actually wants to be rendered, not just
  // rendering it at the fixed --size default. Only ever runs for fonts
  // that actually get drawn (i.e. scroll into view), never the whole
  // list/directory up front - see the module-level comment on
  // FontListBrowser for why.
  #ensureDetail(font) {
    let info = this.#detailCache.get(font.path);
    if(info) return info;

    try {
      const detail = gatherDetailInfo(font.path);
      info = { ...detail, lines: renderDetailLines(detail) };
    } catch(e) {
      info = { meta: null, crisp: null, lines: [`error gathering font info: ${e.message}`] };
    }

    this.#detailCache.set(font.path, info);
    return info;
  }

  #preview(font) {
    const info = this.#ensureDetail(font);
    const size = info.crisp && info.crisp.best ? info.crisp.best.size : this.#previewSize;
    const text = pickSampleText(this.#sampleText, info.meta);
    const key = `${font.path}\0${size}\0${text}`;
    let cached = this.#cache.get(key);

    if(!cached) {
      try {
        cached = { lines: new TerminalGlyphRenderer(font.path, size).renderLines(text) };
      } catch(e) {
        cached = { lines: [`  [unreadable: ${e.message}]`], error: true };
      }
      this.#cache.set(key, cached);
    }

    return cached;
  }

  #entryHeight(font) {
    return 1 + this.#preview(font).lines.length;
  }

  #ensureVisible(availRows) {
    if(this.#selected < this.#scrollTop) this.#scrollTop = this.#selected;

    // eslint-disable-next-line no-constant-condition
    while(true) {
      let used = 0;

      for(let i = this.#scrollTop; i <= this.#selected && i < this.#fonts.length; i++) used += this.#entryHeight(this.#fonts[i]);

      if(used <= availRows || this.#scrollTop >= this.#selected) break;
      this.#scrollTop++;
    }
  }

  // Every row is written via an explicit moveTo(row, 1) + a line clipped to
  // maxCols, never via a trailing '\n' - see clipAnsiLine's comment: this
  // is what makes the layout immune to a terminal's autowrap behavior
  // (GNU screen included) and to being resized mid-render, since each row
  // is positioned fresh from windowSize() every call instead of trusting
  // wherever the cursor happened to land after the previous line.
  #draw(screen, termRows, termCols) {
    const maxCols = Math.max(0, termCols - 1);
    const availRows = termRows - 3;
    this.#ensureVisible(availRows);

    screen.clear(2);
    screen
      .moveTo(1, 1)
      .clearLine(2)
      .write(clipAnsiLine(`fonts: ${this.#fonts.length}   sample: "${this.#sampleText}"   size: ${this.#previewSize}px`, maxCols));
    screen
      .moveTo(2, 1)
      .clearLine(2)
      .write(clipAnsiLine('-'.repeat(Math.min(maxCols, 78)), maxCols));

    let row = 3,
      i = this.#scrollTop;

    while(i < this.#fonts.length && row < termRows - 1) {
      const font = this.#fonts[i];
      const preview = this.#preview(font);
      const mark = this.#marked.has(i) ? '*' : ' ';
      const cursor = i === this.#selected ? '>' : ' ';
      const label = `${cursor}${mark} ${font.path}`;

      screen.moveTo(row, 1).clearLine(2);
      if(i === this.#selected) screen.sgr(7);
      screen.write(clipAnsiLine(label, maxCols));
      if(i === this.#selected) screen.resetAttrs();
      row++;

      for(const line of preview.lines) {
        if(row >= termRows - 1) break;
        screen
          .moveTo(row, 1)
          .clearLine(2)
          .write(clipAnsiLine('    ' + line, maxCols));
        row++;
      }

      i++;
    }

    // Clear any rows left over from a previous, taller render (e.g. after
    // scrolling to a font with fewer preview lines, or a resize).
    for(; row < termRows; row++) screen.moveTo(row, 1).clearLine(2);

    screen.moveTo(termRows, 1).clearLine(2);
    screen.write(clipAnsiLine(this.#status || 'up/down move   space mark   enter detail   t sample text   q quit', maxCols));
    screen.flush();
  }

  #move(delta) {
    this.#selected = Math.max(0, Math.min(this.#fonts.length - 1, this.#selected + delta));
    this.#status = '';
  }

  #promptSampleText(screen, termRows, termCols) {
    let buf = this.#sampleText;
    const maxCols = Math.max(0, termCols - 1);

    // eslint-disable-next-line no-constant-condition
    while(true) {
      screen
        .moveTo(termRows, 1)
        .clearLine(2)
        .write(clipAnsiLine('sample text: ' + buf, maxCols))
        .flush();

      const key = readKey(std.in.fileno());

      if(key.type === 'enter') {
        if(buf.length) this.#sampleText = buf;
        break;
      }
      if(key.type === 'escape' || key.type === 'ctrlc' || key.type === 'eof') break;
      if(key.type === 'backspace') buf = buf.slice(0, -1);
      else if(key.type === 'space') buf += ' ';
      else if(key.type === 'char') buf += key.char;
    }

    this.#status = '';
  }

  // Builds (once per font, cached) the state a glyph-map view pans over:
  // the font's covered codepoints, a uniform cell width, and a TextStyle
  // at findCrispSize()'s best-guess size (so the map visually confirms
  // whether that size actually looks right) - falling back to the
  // browse-mode preview size if no crisp size was found.
  #prepareGlyphMap(font, info) {
    let gm = this.#glyphMapCache.get(font.path);
    if(gm) return gm;

    const size = info.crisp && info.crisp.best ? info.crisp.best.size : this.#previewSize;

    try {
      const style = new TextStyle(font.path, size);
      const codepoints = flattenCoveredCodepoints(info.meta);
      const cellWidth = computeGlyphMapCellWidth(style, codepoints);
      const goodSizes = goodSizesFor(info.crisp, size);

      gm = { style, size, codepoints, cellWidth, goodSizes, rowOffset: 0, colOffset: 0 };
    } catch(e) {
      gm = { error: e.message };
    }

    this.#glyphMapCache.set(font.path, gm);
    return gm;
  }

  // Applies a candidate glyph-map font size, refusing (leaving `gm`
  // unchanged) if it can't be constructed, or if it would leave fewer
  // than 2 glyph cells visible at the current terminal width - the size
  // control is otherwise happy to zoom a font map right off the edge of
  // usefulness, per direct instruction to never let that happen.
  #tryChangeGlyphMapSize(font, gm, newSize, termCols) {
    newSize = Math.max(2, newSize);
    if(newSize === gm.size) return false;

    let style, cellWidth;

    try {
      style = new TextStyle(font.path, newSize);
      cellWidth = computeGlyphMapCellWidth(style, gm.codepoints);
    } catch(e) {
      return false;
    }

    const maxCols = Math.max(0, termCols - 1);
    if(cellWidth * 2 > maxCols) return false;

    gm.size = newSize;
    gm.style = style;
    gm.cellWidth = cellWidth;
    return true;
  }

  // Draws the 2D-pannable glyph map (GLYPHMAP_COLS=16 codepoints per row,
  // as many rows as fit vertically) as the FULL-SCREEN background content
  // - no footer, no flush, no key handling; #showFontDetail below draws
  // the info panel on top of this and owns the interaction loop, per the
  // "glyph viewer in the back, info panel as an overlay" request. Returns
  // the {visibleGlyphRows, visibleCols, totalRows} the caller needs for
  // pan-key clamping.
  #drawGlyphMapFrame(screen, font, gm, termCols, termRows, colorFn) {
    const maxCols = Math.max(0, termCols - 1);

    if(gm.error || gm.codepoints.length === 0) {
      const msg = gm.error ? `error: ${gm.error}` : 'no covered codepoints to show';
      screen.moveTo(1, 1).clearLine(2).write(clipAnsiLine(msg, maxCols));
      return { visibleGlyphRows: 0, visibleCols: 0, totalRows: 0 };
    }

    const totalRows = Math.ceil(gm.codepoints.length / GLYPHMAP_COLS);
    const rowsPerGlyphRow = Math.ceil(gm.size / 2);
    const headerRows = 1;
    const availRows = Math.max(1, termRows - headerRows - 1);
    const visibleGlyphRows = Math.max(1, Math.floor(availRows / rowsPerGlyphRow));
    const visibleCols = Math.max(1, Math.min(GLYPHMAP_COLS - gm.colOffset, Math.floor(maxCols / gm.cellWidth)));

    gm.rowOffset = Math.max(0, Math.min(gm.rowOffset, totalRows - 1));
    gm.colOffset = Math.max(0, Math.min(gm.colOffset, GLYPHMAP_COLS - 1));

    screen
      .moveTo(1, 1)
      .clearLine(2)
      .write(
        clipAnsiLine(
          `glyph map: ${path.basename(font.path)} @ ${gm.size}px   row ${gm.rowOffset + 1}/${totalRows}   cols ${gm.colOffset}-${gm.colOffset + visibleCols - 1}/${GLYPHMAP_COLS - 1}`,
          maxCols,
        ),
      );

    let termRow = 1 + headerRows;

    for(let r = 0; r < visibleGlyphRows; r++) {
      const glyphRowIndex = gm.rowOffset + r;
      if(glyphRowIndex >= totalRows) break;

      const startIndex = glyphRowIndex * GLYPHMAP_COLS + gm.colOffset;
      const lines = renderGlyphMapRow(gm.style, gm.size, gm.cellWidth, gm.codepoints, startIndex, visibleCols, colorFn);

      for(const line of lines) {
        if(termRow >= termRows - 1) break;
        screen.moveTo(termRow, 1).clearLine(2).write(clipAnsiLine(line, maxCols));
        termRow++;
      }
    }

    for(; termRow < termRows; termRow++) screen.moveTo(termRow, 1).clearLine(2);

    return { visibleGlyphRows, visibleCols, totalRows };
  }

  // Draws the font-info panel (overview/render-size/coverage - see
  // gatherDetailInfo()/renderDetailLines()) as a bordered, centered
  // overlay box on top of whatever's already in the buffer - this is
  // exactly Screen.box()'s "later writes in the same frame occlude
  // earlier ones" contract, no different from any other Screen drawing.
  #drawInfoPanel(screen, info, termCols, termRows) {
    const width = Math.min(termCols - 2, 70);
    const height = Math.min(termRows - 2, info.lines.length + 2);
    const { row, col } = centeredRect(termCols, termRows, width, height);
    const innerWidth = Math.max(0, width - 4);

    // box() fills its interior with whatever bg is already active, and
    // draws its border/title with whatever fg is active - setting both
    // first is what makes this a pastel panel instead of a plain-terminal
    // one, and (since it's a later write in the same buffered frame than
    // the glyph map behind it) is also what makes it occlude that map.
    screen.bg(...PANEL_BG_RGB).fg(...PANEL_BORDER_RGB);
    screen.box(row, col, width, height, { title: '◈ font info' });

    let r = row + 1;
    for(const line of info.lines) {
      if(r >= row + height - 1) break;
      screen
        .moveTo(r, col + 2)
        .fg(...PANEL_TEXT_RGB)
        .bg(...PANEL_BG_RGB)
        .write(clipAnsiLine(line, innerWidth));
      r++;
    }

    screen.resetAttrs();
  }

  // The merged font-info + glyph-map view (Enter from the list): the
  // glyph map fills the whole screen and stays pannable throughout ('i'
  // just toggles whether the info panel is drawn on top of it), rather
  // than the two being separate full-screen pages - per direct
  // instruction, "font-info and glyph viewer should really share the
  // screen, with font-info being a panel overlay and the glyph viewer in
  // the back". Esc/Enter/q returns to the font list.
  #showFontDetail(font) {
    const info = this.#ensureDetail(font);
    const gm = this.#prepareGlyphMap(font, info);
    const screen = new Screen();
    let showPanel = true;
    let colorHue = null; // null = plain grayscale; a number = color-wheel mode at that hue

    // eslint-disable-next-line no-constant-condition
    while(true) {
      const [termCols, termRows] = windowSize();
      const maxCols = Math.max(0, termCols - 1);
      const colorFn = colorHue === null ? undefined : colorWheelPalette(colorHue);

      screen.clear(2);
      const { visibleGlyphRows, totalRows } = this.#drawGlyphMapFrame(screen, font, gm, termCols, termRows, colorFn);
      if(showPanel) this.#drawInfoPanel(screen, info, termCols, termRows);

      screen.moveTo(termRows, 1).clearLine(2).write(clipAnsiLine('arrows/pgup/pgdn pan   s/b size   S/B fine size   i info panel   c color mode   esc/enter/q back', maxCols));
      screen.flush();

      const key = readKey(std.in.fileno());

      if(key.type === 'up') gm.rowOffset = Math.max(0, gm.rowOffset - 1);
      else if(key.type === 'down') gm.rowOffset = Math.min(Math.max(0, totalRows - 1), gm.rowOffset + 1);
      else if(key.type === 'pageup') gm.rowOffset = Math.max(0, gm.rowOffset - visibleGlyphRows);
      else if(key.type === 'pagedown') gm.rowOffset = Math.min(Math.max(0, totalRows - 1), gm.rowOffset + visibleGlyphRows);
      else if(key.type === 'left') gm.colOffset = Math.max(0, gm.colOffset - 1);
      else if(key.type === 'right') gm.colOffset = Math.min(GLYPHMAP_COLS - 1, gm.colOffset + 1);
      else if(key.type === 'char' && key.char === 'i') showPanel = !showPanel;
      else if(key.type === 'char' && key.char === 'c') {
        if(colorHue === null) {
          colorHue = this.#nextHue;
          this.#nextHue = (this.#nextHue + 137.508) % 360;
        } else {
          colorHue = null;
        }
      }
      // Size cycling: 's'/'b' (smaller/bigger) coarse-cycle through
      // goodSizesFor()'s tried/2x/4x list; 'S'/'B' (shift held) fine-step
      // by 0.5px, floored at 2px. Deliberately plain letters, not
      // punctuation - a Swiss German QWERTZ keyboard (and many other
      // non-US layouts) doesn't put '+'/'='/'_' where a US layout does
      // (e.g. '=' needs AltGr, '_' needs AltGr+Shift), so the original
      // -/=/+/_ bindings were effectively unreachable there. Letters are
      // layout-stable (same key, same character) except y/z, which this
      // avoids; Shift+letter reliably capitalizes on every layout, so the
      // "hold shift for fine adjustment" gesture the coarse/fine split
      // wants still works, just relocated off punctuation. Both refuse
      // (leave gm.size unchanged) if the result would show fewer than 2
      // glyph cells - see #tryChangeGlyphMapSize().
      else if(key.type === 'char' && key.char === 'b') {
        const target = gm.goodSizes.find(s => s > gm.size);
        if(target !== undefined) this.#tryChangeGlyphMapSize(font, gm, target, termCols);
      } else if(key.type === 'char' && key.char === 's') {
        const target = [...gm.goodSizes].reverse().find(s => s < gm.size);
        if(target !== undefined) this.#tryChangeGlyphMapSize(font, gm, target, termCols);
      } else if(key.type === 'char' && key.char === 'B') {
        this.#tryChangeGlyphMapSize(font, gm, gm.size + 0.5, termCols);
      } else if(key.type === 'char' && key.char === 'S') {
        this.#tryChangeGlyphMapSize(font, gm, gm.size - 0.5, termCols);
      } else if(key.type === 'escape' || key.type === 'enter' || key.type === 'ctrlc' || key.type === 'eof') break;
      else if(key.type === 'char' && key.char === 'q') break;
    }
  }

  // Enables raw mode + the alternate screen buffer, runs the interactive
  // loop, and always restores the terminal on the way out (even on an
  // uncaught error) - returns the set of marked font indices.
  run() {
    const fd = std.in.fileno();

    try {
      setAlternateScreen();
      cursorHide();
      enableRawMode(fd);
      this.#loop();
    } finally {
      // Each restore step run independently - a failure in one (e.g.
      // disableRawMode's tcgetattr on a non-tty stdin) must not skip the
      // rest, or the terminal is left alt-screen/cursor-hidden/raw.
      for(const restore of [() => disableRawMode(fd), cursorShow, setNormalScreen]) {
        try {
          restore();
        } catch(e) {}
      }
    }

    return this.#marked;
  }

  #loop() {
    const screen = new Screen();
    let running = true;

    while(running) {
      const [cols, rows] = windowSize();
      this.#draw(screen, rows, cols);

      const key = readKey(std.in.fileno());

      switch (key.type) {
        case 'up':
          this.#move(-1);
          break;
        case 'down':
          this.#move(1);
          break;
        case 'pageup':
          this.#move(-5);
          break;
        case 'pagedown':
          this.#move(5);
          break;
        case 'space':
          if(this.#marked.has(this.#selected)) this.#marked.delete(this.#selected);
          else this.#marked.add(this.#selected);
          break;
        case 'enter':
          this.#showFontDetail(this.#fonts[this.#selected]);
          break;
        case 'char':
          if(key.char === 'q') running = false;
          else if(key.char === 't') this.#promptSampleText(screen, rows, cols);
          break;
        case 'ctrlc':
        case 'escape':
        case 'eof':
          running = false;
          break;
      }
    }
  }
}

// ============================================================================
// SECTION: export-view
// ============================================================================
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
// Unchanged from the original single-font version of this tool - kept as
// its own section/function so a future Stage 4 ("export the fonts marked in
// the browser") can call it per selected font.

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
  if(layout === 'row') return { cols: n, rows: 1 };
  if(layout === 'column') return { cols: 1, rows: n };
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
//
// blockinessScore/distinctMidgrayLevels/BITMAP_LEVEL_THRESHOLD now live in
// the info-gathering section above - findCrispSize() there needs the same
// antialiasing-level measurement this size-sanity check does, just applied
// across a range of candidate sizes instead of the one size actually used
// for export.
//
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
    console.log(
      `size sanity: WARNING - bitmap font render has ${levels} distinct antialiased levels (expected < ${BITMAP_LEVEL_THRESHOLD}) - looks mis-scaled despite matching fc-query's native size`,
    );
    return;
  }

  console.log(`size sanity: OK (bitmap font, crisp render at ${fontSize}px, ${levels} distinct antialiased levels)`);
}

function exportFontSheet(fontFile, { size, start, end, layout, out } = {}) {
  const meta = queryFontMetadata(fontFile);

  const fontSize = size ? parseInt(size, 10) : (meta && meta.pixelSize) || 16;
  const startCode = start ? parseCode(start) : 0;
  const endCode = end ? parseCode(end) : meta ? meta.highestCodepoint + 1 : 127;
  const fontName = path.basename(fontFile, path.extname(fontFile));
  const outBase = out || `${fontName}@${fontSize}`;

  if(meta) console.log(`fc-query: pixelSize=${meta.pixelSize ?? '(scalable)'} codepoints=U+${meta.lowestCodepoint.toString(16)}..U+${meta.highestCodepoint.toString(16)}`);

  const style = new TextStyle(fontFile, fontSize);
  const n = endCode - startCode;

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

  for(let code = startCode; code < endCode; code++) {
    if(meta && !isCovered(meta.ranges, code)) continue;

    const ch = String.fromCodePoint(code);
    let glyphDescent;
    const sz = style.size(ch, y => (glyphDescent = y));

    if(isBogus(sz.height, glyphDescent)) {
      bogusCount++;
      cellWidth = Math.max(cellWidth, sz.width);
      metrics.push({ code, ch, glyphAscent: 0, glyphDescent: 0 });
      continue;
    }

    cellWidth = Math.max(cellWidth, sz.width);
    ascent = Math.max(ascent, sz.height);
    descent = Math.max(descent, glyphDescent);
    metrics.push({ code, ch, glyphAscent: sz.height });
  }

  if(metrics.length === 0) throw new Error(`no codepoint in U+${startCode.toString(16)}..U+${(endCode - 1).toString(16)} is covered by this font`);

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
    const index = code - startCode;
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
    start: startCode,
    end: endCode,
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
}

// ============================================================================
// main
// ============================================================================

function main() {
  // No args array: the constructor then reads the global `scriptArgs`
  // (script path included), matching the argv[0]-is-the-program-name
  // convention the parser expects - passing scriptArgs.slice(1) instead
  // makes it treat the first real option as the program name and silently
  // drop it.
  const parser = new CommandLineParser(KEYS);
  parser.about('Font gathering/browsing/export tool.');

  if(!parser.check()) {
    parser.printErrors();
    return 1;
  }

  if(parser.has('help')) {
    parser.printMessage();
    return 0;
  }

  const dirs = parser.has('dirs') ? splitList(parser.get('dirs')) : [];
  const fclistFilters = parser.has('fclist') ? splitList(parser.get('fclist')) : [];
  const files = parser.has('fonts') ? splitList(parser.get('fonts')) : [];

  // The positional arg is a font FILE (legacy export mode, unless other
  // browse-mode flags are also given) or a DIRECTORY (browse mode, walked
  // recursively - same as passing it via --dirs). Never fed straight to
  // the font loader either way: that's what previously made `qjsm
  // render_font.js some/directory/` try to open the directory itself as a
  // font file and blow up with an unhelpful FreeType/OpenCV assertion.
  let legacyFont = null;

  if(parser.has('@font')) {
    const arg = parser.get('@font');
    let st = null;

    try {
      st = statSync(arg);
    } catch(e) {
      console.log(`error: can't stat '${arg}': ${e.message}`);
      return 1;
    }

    if(st.isDirectory()) dirs.push(arg);
    else legacyFont = arg;
  }

  const browseMode = dirs.length > 0 || fclistFilters.length > 0 || files.length > 0;

  if(browseMode) {
    if(legacyFont) files.push(legacyFont);

    const fonts = FontCatalog.gather({ files, dirs, fclistFilters });

    if(fonts.length === 0) {
      console.log('no fonts found from the given --dirs/--fclist/--fonts sources');
      return 1;
    }

    const sampleText = parser.get('text');
    const previewSize = parser.get('size') ? parseInt(parser.get('size'), 10) : 14;

    // FontListBrowser now detects each font's crisp render size (and a
    // sample text that avoids missing-glyph "tofu" boxes) as soon as it
    // draws that font's row, not only once its detail view is opened -
    // "reading the font list" now includes that detection. It's still
    // lazy/on-demand per row (never the whole directory up front - see
    // #ensureDetail's comment), but the first screenful's rows haven't
    // been drawn yet, so there's an unavoidable one-time pause right
    // after entering the list for however many rows fit on screen.
    console.log(`opening ${fonts.length} font(s) - detecting render size for the first screenful...`);

    new FontListBrowser(fonts, { sampleText, previewSize }).run();
    return 0;
  }

  if(!legacyFont) {
    parser.printMessage();
    return 1;
  }

  try {
    exportFontSheet(legacyFont, {
      size: parser.get('size'),
      start: parser.get('start'),
      end: parser.get('end'),
      layout: parser.get('layout'),
      out: parser.get('out'),
    });
  } catch(e) {
    console.log('error:', e.message);
    return 1;
  }

  return 0;
}

const code = main();
if(code) std.exit(code);

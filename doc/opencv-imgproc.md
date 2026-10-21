# opencv-imgproc: CLI spec for background removal and clip paths

Status: spec, 2026-10-21. Owner of the code: the `qjs-opencv` project
(`~/Sources/quickjs/qjs-opencv`, script `tools/opencv-imgproc.js`). Consumer:
the Wild Beauty Bun server (`server/media/imgproc.js`, `PLAN.md` S10). This
file is the contract between the two; the qjs-opencv copy lives at
`doc/opencv-imgproc.md` and the two are kept identical.

## 1. What it is

One script, run as `qjsm opencv-imgproc.js <command> ...`, that does the
pixel work a web server cannot do with ImageMagick one-liners:

- **cutout**: remove the background of a photo by any of several methods
  (grabCut, border flood fill, chroma key, k-means, watershed, contour,
  difference to a reference image, an ONNX segmentation model, a supplied
  mask/trimap, or `auto`) and write an image **with an alpha channel**.
- **clip**: cut a photo with a polygon or SVG path (fill the shape into a mask,
  apply the mask as alpha), optionally inverted, optionally trimmed.
- **alpha**: apply an existing mask image as alpha.
- **info**, **capabilities**: describe an image; describe what this install
  can do (methods, models, limits).

It is a *program*, not a library: input files in, one output file and one JSON
line out. The server runs it as a subprocess with an argv array (never a shell).

Non-goals: video, batch directories, a GUI, handwriting, resizing/format
conversion beyond what cutout/clip need, HEIC decoding (the server converts
HEIC/HEIF to jpg/png first, `PLAN.md` S6.2 step 3b), any network access.

## 2. Environment and facts checked on 2026-10-21

Verified on this machine with `QUICKJS_MODULE_PATH=<qjs-opencv>/build/x86_64-linux-gnu`
and the installed `qjsm`:

- `import * as cv from 'opencv'` works; OpenCV reports `5.1.0-dev`; the module
  exports 1252 names.
- Bound and usable for this job: `grabCut` (+ `GC_*`, tested by
  `tests/test_grabcut.js`), `floodFill`, `watershed`, `kmeans`, `threshold`,
  `inRange`, `cvtColor`, `GaussianBlur`, `bilateralFilter`, `morphologyEx`,
  `distanceTransform`, `connectedComponentsWithStats`, `findContours`,
  `drawContours`, `bitwise_*`, `merge`/`split`, `copyTo`, `resize`, `Canny`,
  `imread`/`imwrite`/`imdecode`/`imencode` (`.png`, `.webp`, `.jpg` all
  encode), `readNetFromONNX`, `blobFromImage`, `createBackgroundSubtractor{MOG2,KNN}`.
- **Not bound**: `StaticSaliency*`, `guidedFilter`; `getBuildInformation`.
  So: no OpenCV saliency method and no guided-filter matting; implement what is
  wanted in JS or leave the method out of `capabilities`.
- **Found broken or odd, to be settled first (task 1)**:
  - `cv.fillPoly(mat, pts, color[, lineType])` throws `[unsupported type]` with
    all of: array of arrays of `Point`, flat array of `Point`, flat
    `{x,y}` objects, flat `[x,y]` pairs, with a `Scalar` or an array colour
    (`js_draw.cpp:392 js_fill_poly` reads *one* flat polygon through
    `point_vector<int>` and then a colour). `cv.fillConvexPoly` fails its own
    assertion (`points.checkVector(2, CV_32S) >= 0`), so it wants an `int32`
    two-column `Mat`. `cv.drawContours(m, [pts], 0, color, -1)` returns without
    error but the check showed no pixels written. Find the call form that works
    (an `int32` `Mat` of points is the first thing to try), or fix the binding,
    and log it in `BUGS` per that repo's convention. Fallback that always
    works: a small JS scanline polygon rasteriser into a `Uint8Array` wrapped
    in a `Mat`. Whatever is chosen, clip results must be sub-pixel accurate (S6).
  - `Mat.channels()` is a method (calling it returns `4` for `CV_8UC4`).
  - `imread` applies EXIF orientation unless told not to; `imwrite` of an
    8-bit BGRA `Mat` to `.png` round-trips with 4 channels
    (`IMREAD_UNCHANGED`).
  - The script must read `doc/opencv-js-api.md` and `doc/vector-arguments.md`
    for argument shapes instead of guessing.

## 3. Conventions (from the `quickjs-scripting` skill)

- File `tools/opencv-imgproc.js`, ES module, top-of-file banner stating it is
  **quickjs-only** (it needs the `opencv` module). It imports nothing from
  `std` or `os`: `fs`, `path`, `process` (and `perf_hooks` for timing) only.
  Arguments: `process.argv.slice(2)`. Exit with `process.exit(n)`.
- Comments follow the `comment-style` skill (`JS | script | ...` rulesets).
- Run: `qjsm opencv-imgproc.js <command> [options]` with the module path in
  `QUICKJS_MODULE_PATH` (absolute paths, see the skill).
- **stdout is exactly one line of JSON** (the result, S8) and nothing else;
  diagnostics go to `process.stderr.write`, never `console.log` (qjsm older
  than 2026-10-20 sends `console.error` to stdout).
- Exit status: 0 ok, otherwise a code from S9. On failure stdout still holds
  one JSON line `{"ok":false,"error":{...}}`.
- Deterministic: same inputs and seed give the same bytes
  (`cv.setRNGSeed(seed)`, default 0; k-means and grabCut are the random ones).
- No writes except `--out`, `--mask-out` and temp files next to `--out`
  (`<out>.tmp-<pid>`, renamed on success, removed on failure).
- Register the tool in `tool-registry` (`tools.yaml`, with the user's
  approval) and add a `qjs-opencv` README section.

## 4. Commands

```
opencv-imgproc.js capabilities
opencv-imgproc.js info    --in FILE
opencv-imgproc.js cutout  --in FILE --out FILE [--method M] [options] | --job JOB.json
opencv-imgproc.js clip    --in FILE --out FILE (--polygon JSON | --path D | --mask FILE) [options] | --job JOB.json
opencv-imgproc.js alpha   --in FILE --out FILE --mask FILE [options]
opencv-imgproc.js --help | <command> --help
```

`--job FILE` reads the same options as one JSON object (S5); command-line
flags override it. The server always uses `--job` (no quoting issues, no
argv length limit). Flag names are the JSON keys, kebab-case (`--max-pixels`
= `maxPixels`); booleans are `--flag`/`--no-flag`; repeated flags make arrays.

### 4.1 `capabilities`

Prints (and exits 0, even when degraded):

```json
{ "ok": true, "tool": "opencv-imgproc", "version": "1.0.0",
  "opencv": "5.1.0-dev",
  "methods": ["auto","grabcut","flood","chroma","threshold","kmeans",
              "watershed","contour","diff","dnn","mask"],
  "unavailable": { "saliency": "StaticSaliency not bound" },
  "models": [ { "name": "u2netp", "file": "...", "input": [320,320] } ],
  "inputFormats": ["png","jpg","webp","tiff"],
  "outputFormats": ["png","webp"],
  "limits": { "maxPixels": 64000000, "workSize": 1280 } }
```

`methods` lists only what actually runs on this install (a method whose
binding or model is missing goes to `unavailable` with the reason).
`inputFormats` is probed by decoding tiny in-memory samples, not hard-coded.

### 4.2 `info`

`{ "ok":true, "width":W, "height":H, "channels":1|3|4, "hasAlpha":bool,
"depth":8|16|32, "orientationApplied":1-8|null, "alphaCoverage":0..1|null }`.
`alphaCoverage` = fraction of pixels with alpha > 0 (null without alpha).

### 4.3 `palette`

`opencv-imgproc palette --in=FILE [--count=12] [--dist=cube|cie76|cie94|kmeans] [--hsv]`
returns the dominant colours of an image (`dominant_colors_grabber`, through
`cv.paletteGenerate`; the image is first reduced to a 512 px long edge):

```json
{ "ok": true, "command": "palette", "space": "bgr",
  "input": { "width": 600, "height": 794, "channels": 4 },
  "colors": ["#8ad966", "#000000", "#fe2943"] }
```

`colors` are `#rrggbb` strings in the algorithm's order, without weights; with `--hsv`
they are raw `[h, s, v]` triplets and `space` is `"hsv"`. Implementation note: the tool
parses its options with `cv.CommandLineParser`, so options are `--name=value`
(not `--name value`) and there is no `--job` file yet.

## 5. Job schema

Common to `cutout`, `clip`, `alpha`:

| Key | Type | Default | Meaning |
|---|---|---|---|
| `in` | path | required | input image |
| `out` | path | required | output image; extension chooses the format |
| `maskOut` | path | none | also write the final 8-bit mask as grey PNG |
| `maxPixels` | int | 64000000 | reject larger inputs (`E_TOO_LARGE`) |
| `workSize` | int | 1280 | long edge for analysis; the mask is computed at this size and upscaled (S6.4) |
| `seed` | int | 0 | RNG seed |
| `trim` | bool | false | crop output to the alpha bounding box (+ `trimPad` px) |
| `trimPad` | int | 0 | padding for `trim` |
| `format` | `"png"` \| `"webp"` | from `out` | overrides the extension check |
| `webp` | `"lossless"` \| int 1-100 | `"lossless"` | WebP quality |
| `png` | int 0-9 | 6 | PNG compression |
| `post` | object | see S6.5 | mask post-processing |

### 5.1 `cutout`

| Key | Type | Default | Meaning |
|---|---|---|---|
| `method` | string | `"auto"` | one of `capabilities.methods` |
| `rect` | `[x,y,w,h]` or `[x0,y0,x1,y1]` with `"rectUnits"` | centre 80% | subject box (grabCut, watershed, contour) |
| `rectUnits` | `"px"` \| `"norm"` | `"px"` | `norm` = 0..1 of width/height |
| `fg` | array of polygons/strokes | none | "this is subject" hints (S6.2) |
| `bg` | array of polygons/strokes | none | "this is background" hints |
| `trimap` | path | none | grey image: 0 bg, 128 unknown, 255 fg; used by `mask` and as seed by others |
| `grabcut` | `{iters:5, mode:"rect"\|"mask"\|"auto"}` | | `auto` = rect from border-vs-centre colour statistics |
| `flood` | `{tolerance:12, space:"lab", seeds:"corners"\|"border"\|[[x,y],...], connectivity:8}` | | |
| `chroma` | `{key:"auto"\|"#rrggbb", space:"hsv"\|"lab", tolerance:30, soft:15, despill:true}` | | `auto` = dominant colour of the 4 px border |
| `threshold` | `{channel:"gray"\|"l"\|"s"\|"v"\|"a"\|"b", mode:"otsu"\|"adaptive"\|int, invert:"auto"\|bool}` | | |
| `kmeans` | `{k:5, space:"lab", bgClusters:"border"\|[ids]}` | | clusters dominating the border are background |
| `watershed` | `{markers:"auto"}` | | markers from `fg`/`bg`/`rect`: inside eroded rect = fg, outside the rect and the border = bg |
| `contour` | `{canny:[50,150], close:5, pick:"largest"\|"central"}` | | edges, close gaps, fill the chosen contour |
| `diff` | `{reference:path\|[paths], model:"absdiff"\|"mog2"\|"knn", threshold:25, align:false}` | | difference to an empty-scene image |
| `dnn` | `{model:"u2netp", threshold:0.5, invert:false}` | | model from `--models-dir` (S6.3) |
| `modelsDir` | path | env `WB_MODELS_DIR` | where `*.onnx` + `*.json` live |

### 5.2 `clip`

| Key | Type | Default | Meaning |
|---|---|---|---|
| `polygon` | `[[x,y],...]` or `[[[x,y],...], ...]` | none | one or several polygons |
| `path` | string | none | SVG path data (S7.2) |
| `mask` | path | none | use this mask image instead of drawing |
| `units` | `"px"` \| `"norm"` \| `"box"` | `"px"` | coordinate space of the shape (S7.1) |
| `box` | `[x,y,w,h]` | `[0,0,1000,1000]` | for `units:"box"`: the shape's own square (node space) |
| `matrix` | `[a,b,c,d,e,f]` | identity | affine map from shape space to image pixels, applied after `units` |
| `fillRule` | `"evenodd"` \| `"nonzero"` | `"nonzero"` | for overlapping subpaths |
| `invert` | bool | false | keep the outside, cut the inside |
| `mode` | `"replace"` \| `"intersect"` \| `"add"` \| `"subtract"` | `"intersect"` | how the shape mask combines with the image's existing alpha |
| `feather` | number px | 0 | Gaussian blur of the mask edge |
| `grow` | number px | 0 | dilate (+) or erode (-) the mask before feathering |
| `supersample` | int 1-8 | 4 | edge quality (S7.3) |

### 5.3 `alpha`

`--mask FILE` (grey image, resized with area/linear filter to the input if
sizes differ, and `E_MASK_SIZE` if aspect ratios differ by more than 1%),
plus `mode`, `invert`, `feather`, `grow` as in `clip`.

## 6. Cutout methods and post-processing

### 6.1 Common pipeline

1. Read the input (S9 for errors). Apply EXIF orientation (default `imread`
   behaviour) and report it in the result. Convert 16-bit/float to 8-bit.
   Drop any existing alpha for analysis but remember it: a cutout on an image
   that already has alpha intersects with the old alpha (`mode:"intersect"`
   semantics), never resurrects transparent pixels.
2. Make a work copy: long edge `workSize` (area interpolation), BGR.
3. Run the method; it yields a **probabilistic or binary foreground mask**
   (8-bit, 255 = subject) at work size.
4. Seeds: `fg`/`bg` hints and `trimap` are honoured by every method that can
   (grabCut `GC_FGD/GC_BGD`, watershed markers, flood barrier, kmeans cluster
   assignment); methods that cannot ignore them and say so in `warnings`.
5. Post-process (S6.5) at work size, upscale to full size (S6.4), compose the
   output.

### 6.2 Seeds and hints

A hint is a polygon `[[x,y],...]` (filled), a stroke `{"points":[[x,y],...],
"width":px}` (drawn thick), or a circle `{"c":[x,y],"r":px}`; coordinates follow
`rectUnits`. They are rasterised at work size.

### 6.3 Methods

| `method` | How | Good for | Notes |
|---|---|---|---|
| `grabcut` | `cv.grabCut` initialised by `rect` (default centre 80%), by mask (hints/trimap), or `auto`; `iters` default 5; probable fg/bg from the result | general photos, subject roughly centred | slow on big inputs, hence `workSize`; with `trimap`, uses `GC_INIT_WITH_MASK` |
| `flood` | `floodFill` from the border (or given seeds) with a Lab tolerance, mask = everything not reached; fill holes | product shots on a plain/gradient backdrop, scans, documents | `floodFill` with `FLOODFILL_MASK_ONLY \| FLOODFILL_FIXED_RANGE` off so gradients are followed |
| `chroma` | key colour distance in HSV or Lab with a hard `tolerance` and a soft ramp `soft`; optional despill (pull key tint out of edge pixels) | green/blue/white screens | `key:"auto"` picks the dominant border colour |
| `threshold` | Otsu or adaptive threshold on one channel; `invert:"auto"` makes the larger border-connected region the background | silhouettes, line art, scans | |
| `kmeans` | `cv.kmeans` on Lab (subsampled), clusters that dominate the image border are background | backgrounds with texture/gradient but distinct colour | `seed` matters; cap iterations |
| `watershed` | `cv.watershed` with markers from hints or the rect | touching objects, clean boundaries | |
| `contour` | Canny, morphological close, `findContours`, fill the largest (or most central) contour, then hole-fill | clear outlines on busy backdrops | |
| `diff` | `absdiff` against `reference` (or MOG2/KNN trained on several references), threshold, clean up | studio setups where an empty shot exists | `align` = ECC/ORB homography; off by default |
| `dnn` | `readNetFromONNX` + `blobFromImage`, run, sigmoid, resize the matte back, threshold or keep soft | best quality when a model is installed | model definition S6.3.1; no model ships with the tool |
| `mask` | uses `trimap`/`--mask` directly; unknown band (128) is resolved by a small grabCut | user-drawn or externally computed masks | |
| `auto` | see S6.3.2 | default | |

Methods not implementable with the current bindings (saliency, guided-filter
matting) are not offered (S2); do not fake them.

#### 6.3.1 DNN model sidecar

`<models-dir>/<name>.onnx` plus `<name>.json`:

```json
{ "input":  { "size": [320, 320], "mean": [0.485,0.456,0.406],
              "std": [0.229,0.224,0.225], "scale": 0.00392157, "rgb": true },
  "output": { "index": 0, "activation": "sigmoid", "normalize": "minmax" },
  "license": "Apache-2.0" }
```

The tool lists a model in `capabilities` only if `readNetFromONNX` loads it
(OpenCV's ONNX operator coverage is limited; a model that does not load is
reported under `unavailable` with OpenCV's message). Common candidates: U^2-Net
/ U^2-Net-P, ISNet, MODNet, RMBG-1.4 (check each licence before bundling:
several are non-commercial). Inference is CPU, so cap input size and report
`timings.dnnMs`.

#### 6.3.2 `auto`

1. If a `dnn` model is available, run it first.
2. Otherwise measure the border (the outer 2% ring): if its colour variance
   (Lab) is low (flat backdrop), run `flood`; else run `grabcut` (`rect:auto`).
3. Score the candidate mask: subject area between 2% and 98%, one dominant
   connected component, smooth boundary (perimeter/area vs. its convex hull),
   little mass on the border. If the score is below a threshold, try the next
   method in the order `flood, grabcut, kmeans, contour`; keep the best.
4. Report `chosen` and `scores` in the result.

### 6.4 Upscaling the mask

Compute at work size, then upscale to full size with bicubic, then, if
`post.refine` is on (default when work size < full size), run a narrow-band
refinement: build a trimap from the upscaled mask (band = `refineBand` px
around the edge, default 6), and resolve the band with a grabCut pass limited
to the band's bounding boxes at full resolution (or a bilateral/joint-filter
edge snap if that is faster); never process the whole full-size image with
grabCut.

### 6.5 Post-processing (`post`)

Applied in this order; every step has a neutral default; defaults shown.

| Key | Default | Meaning |
|---|---|---|
| `binarize` | `0.5` for soft masks | threshold (0..1); `null` keeps the soft matte |
| `open` | 1 | morphological open radius px (removes specks) |
| `close` | 2 | close radius px (fills pinholes) |
| `keep` | `"largest"` | `"largest"`, `"all"`, `"central"`, or an integer (that many biggest components) |
| `minArea` | 0.002 | drop components below this fraction of the image |
| `fillHoles` | `true` | fill interior holes (set false for rings/frames) |
| `grow` | 0 | px; negative erodes (shrink away from halos) |
| `feather` | 1.0 | Gaussian sigma px on the mask edge |
| `refine` | auto | S6.4 |
| `decontaminate` | `true` | RGB of pixels with alpha 0 is replaced by the nearest opaque colour (a few dilation passes), so lossy resampling/WebP and CSS scaling do not show halos |

## 7. Clip details

### 7.1 Coordinate spaces

- Image space: origin top-left, x right, y down, a pixel `(i, j)` covers
  `[i, i+1) x [j, j+1)`; polygon coordinates are continuous (a vertex at 10.5
  lies at the centre of pixel 10).
- `units:"px"`: shape coordinates are image pixels.
- `units:"norm"`: 0..1 of width and height.
- `units:"box"`: shape coordinates are inside `box` (default `[0,0,1000,1000]`,
  the node space of `PLAN.md` S7) and are mapped to the whole image:
  `x_img = (x - box.x) / box.w * W`, `y_img = (y - box.y) / box.h * H`.
- `matrix [a b c d e f]` is applied last: `x' = a*x + c*y + e`,
  `y' = b*x + d*y + f` (SVG `matrix()` convention). The server uses it to
  pass a node's photo transform: if a photo step has matrix `m` placing the
  photo in node space and a clip shape lives in node space, the server sends
  the **inverse of `m`** so the shape lands in the photo's own pixel space.

### 7.2 SVG path grammar

`M m L l H h V v C c S s Q q T t Z z` (required; the editor emits `M`, `C`,
`L`, `Z`); `A a` (optional, converted to cubics). Multiple subpaths allowed;
implicit lineto after `M`; numbers like `.5`, `-1e-2`, `1.5.5` per the SVG
grammar. Curves are flattened by recursive subdivision to a tolerance of 0.25
output pixels. A parse error is `E_BAD_SHAPE` with the offending offset.
Open subpaths are closed implicitly for filling (as SVG does).

### 7.3 Rasterising and edge quality

- Render the mask at `supersample` x the output size in 8-bit (or with
  OpenCV's sub-pixel `shift` argument if `fillPoly` works with it, S2), then
  area-downsample to full size; this gives anti-aliased edges. `supersample`
  is lowered automatically so the work canvas stays under `maxPixels`.
- `nonzero` and `evenodd` are both required. `cv.fillPoly` is even-odd for
  one call with several polygons; for `nonzero`, fill each subpath separately
  into its own mask by winding direction rules or by compositing (documented
  in the code); the tests (S12) pin the behaviour on a figure-eight and on a
  ring.
- Degenerate shapes: fewer than 3 distinct points, zero area, all points
  outside the image, NaN/Infinity: `E_BAD_SHAPE`. A shape fully covering the
  image is legal (output equals input with full alpha).
- `invert` flips the final mask. `grow`/`feather` come last.

### 7.4 Combining with alpha

`mode:"replace"`: alpha = shape mask. `intersect` (default): alpha =
old_alpha x mask (so repeated clipping only ever removes). `add`: max.
`subtract`: old_alpha x (1 - mask). An input without alpha has alpha 255
everywhere before combining.

## 8. Output and result

- The output image **always has 4 channels (BGRA)** after `cutout`, `clip` and
  `alpha`. If the input has no alpha channel (JPEG, HEIC-derived JPEG, opaque
  PNG) one is added. The output extension must be an alpha-capable format:
  `.png` (default) or `.webp`. `.jpg`/`.jpeg`/`.bmp` are rejected with
  `E_FORMAT` and nothing is written (the server never asks for them; the tool
  must not silently flatten the alpha).
- No EXIF/ICC is written (OpenCV does not); orientation is already applied to
  the pixels, which the result reports.
- With `trim`, the output is cropped to the bounding box of alpha > 0 (+
  `trimPad`, clamped); the result carries the crop offset so the server can
  keep the photo's place in a node (`bbox`, `offset`).

One JSON line on stdout:

```json
{ "ok": true, "command": "cutout", "method": "grabcut", "chosen": "grabcut",
  "input":  { "path": "...", "width": 4000, "height": 3000, "channels": 3,
              "hasAlpha": false, "orientationApplied": 6 },
  "output": { "path": "...", "format": "png", "width": 3000, "height": 4000,
              "hasAlpha": true, "bytes": 123456 },
  "mask":   { "path": null, "coverage": 0.41, "components": 1 },
  "bbox":   { "x": 120, "y": 80, "w": 2500, "h": 3700 },
  "offset": { "x": 120, "y": 80 },
  "scores": { "grabcut": 0.82 },
  "warnings": [],
  "timings": { "readMs": 40, "segmentMs": 900, "postMs": 60, "writeMs": 80 } }
```

`coverage` is the fraction of pixels with alpha > 127. `output.width/height`
are after `trim`. `bbox` is in output-before-trim space when `trim` is on
(`offset` = its top-left); otherwise `offset` is `{x:0,y:0}`.

## 9. Errors and exit codes

On error stdout is `{"ok":false,"error":{"code":"E_...","message":"...",
"detail":{...}}}` and the exit code is:

| Exit | Code | When |
|---|---|---|
| 2 | `E_USAGE` | bad flags, unknown method, invalid job JSON, missing required key |
| 3 | `E_INPUT` | input missing, unreadable, zero bytes, not decodable, corrupt |
| 4 | `E_FORMAT` | output format cannot hold alpha, or input format unsupported (HEIC: message says "convert first") |
| 5 | `E_TOO_LARGE` | pixels above `maxPixels` (checked from the header before decoding where possible) |
| 6 | `E_BAD_SHAPE` | clip shape invalid (S7.3) |
| 7 | `E_MASK_SIZE` | supplied mask/trimap does not fit the input |
| 8 | `E_MODEL` | dnn model missing or does not load |
| 9 | `E_METHOD` | method unavailable on this install, or failed to produce a usable mask (empty, or covers > 99.5% / < 0.1% and `strict`) |
| 10 | `E_WRITE` | cannot write `out` |
| 70 | `E_INTERNAL` | uncaught exception (message carries OpenCV's text) |

The server maps 3/4/5/6/7 to a 4xx response (bad input) and 8/9/10/70 to 5xx,
and always deletes any partial output.

## 10. Limits and security

- Only the paths given are touched. `in`/`out`/`mask`/`reference`/`modelsDir`
  are used as is, no globbing, no `~`, no URLs; the server passes absolute
  paths it built from validated hashes.
- Never executes anything, never spawns processes, no network.
- Cap everything an adversarial input can inflate: decoded pixels
  (`maxPixels`), polygon vertex count (default 100000), path length (1 MB),
  hint count, k-means `k <= 16`, grabCut iterations <= 10, `supersample`
  canvas pixels, `feather`/`grow` <= 10% of the long edge.
- Memory: peak is roughly `4 x channels x pixels` x a small constant; keep the
  full-size BGRA, the mask and one temporary only; free `Mat`s explicitly.
- Time: the server enforces the wall-clock timeout (kill the child); the tool
  also takes `budgetMs` (default 60000) and returns `E_METHOD` instead of
  starting another pass when exceeded.

## 11. Server integration (Wild Beauty)

`server/media/imgproc.js` does, with Bun APIs:

```js
const job = { in: srcPath, out: tmpOut, method: 'auto', ...userParams };
await Bun.write(jobPath, JSON.stringify(job));
const p = Bun.spawn(['qjsm', toolPath, 'cutout', '--job', jobPath], {
  env: { QUICKJS_MODULE_PATH: modPath, WB_MODELS_DIR: modelsDir, PATH },
  stdout: 'pipe', stderr: 'pipe', timeout: 70_000, killSignal: 'SIGKILL' });
const result = JSON.parse((await new Response(p.stdout).text()).trim());
```

- `POST /api/uploads/:hash/cutout` validates `:hash` and the user-supplied job
  fields against the tables in S5 (unknown keys dropped, ranges clamped),
  fills `in` with the **display file** of that upload (HEIC already
  extracted), `out` with a temp file under `uploads/.tmp/`.
- On `ok`, the server hashes the output with SHA-256, renames it to
  `uploads/<newhash>.png` (or `.webp`), writes the sidecar like any upload
  plus `derivedFrom: <hash>`, `op: { command, method, params }`, runs the
  normal probe and palette step, and answers with that sidecar. The tool's
  `bbox`/`offset` go into `op` so a node can place the trimmed photo back.
- The same call shape serves the bake step (`clip` with the node's `shape`
  and the inverse photo matrix) for thumbnails and flat exports.
- `/api/capabilities.imgproc` is the output of `capabilities` (or `null` when
  `qjsm` or the module is missing, in which case the client hides the tools).
- The tool and its models are not part of the Wild Beauty repo; their location
  is configuration (`server/config.js`).

## 12. Tests and acceptance

Tests live in `qjs-opencv/tests/` (plain scripts runnable with `qjsm`, fixtures
generated in code with `cv` drawing so no binary files are needed; a few small
real photos may be added under `tests/imgproc/`).

Functional:
1. `capabilities` lists exactly the methods that run; with the module path
   unset the script prints a clear `E_INTERNAL`-style JSON error and exits non-zero.
2. `flood`: a disc on a flat colour with a gradient: mask IoU >= 0.97 with the
   ground-truth disc.
3. `chroma`: subject on `#00ff00` with soft shadow: IoU >= 0.95, no green fringe
   (mean green excess on edge pixels below a threshold with `despill`).
4. `grabcut` with the default rect on a high-contrast subject: IoU >= 0.9.
5. `kmeans`, `threshold`, `contour`, `watershed`, `diff` each pass one
   synthetic case with a stated IoU floor.
6. `dnn`: skipped with a clear message when no model; with a tiny test ONNX
   (identity-like) checks the pre/post-processing plumbing.
7. `clip`: triangle, ring (`evenodd` vs `nonzero`), figure-eight, a cubic
   blob, `invert`, `units:"box"` with a `matrix`: rasterised area within 0.5%
   of the analytic area; edge pixels are anti-aliased (not just 0/255).
8. Alpha handling: JPEG in -> 4-channel PNG out; `.jpg` out -> exit 4 and no
   file; existing alpha is intersected, never resurrected; `trim`,
   `offset` correct; `decontaminate` leaves no dark halo in a WebP round trip.
9. `alpha` with a mask of a different size resizes; different aspect -> exit 7.
10. Determinism: two runs with the same job give byte-identical output.

Hostile / robustness: missing file, zero bytes, truncated JPEG, 1x1 image,
100000x100000 PNG header (rejected before decoding), a polygon with NaN, one
with 10^6 vertices, a 5 MB path string, a `--job` with unknown keys, `out` in a
nonexistent directory.

Performance (informational targets, this machine): a 12 MP JPEG: `flood` and
`chroma` < 2 s, `grabcut` (workSize 1280) < 6 s, `clip` < 1.5 s; memory < 1 GB.

Done when: all of the above pass; `capabilities` and the result JSON match
this file; `doc/opencv-imgproc.md` equals this spec (updated where the
implementation had to differ, with the difference noted); `BUGS` holds every
binding problem found (S2), and `TODO.md` lists what was not done.

## 13. Open items for the implementer

- Pick the working polygon-fill path (S2); decide `nonzero` implementation.
- Check the DNN story (OpenCV 5.1 dev ONNX operator coverage) with one real
  model and report which models load; do not bundle models without a licence check.
- Confirm EXIF orientation behaviour of `imread` on this build (the S2 note
  is about the standard OpenCV default; test with a rotated JPEG).
- Decide whether `align` for `diff` is worth it (default off, may be cut).

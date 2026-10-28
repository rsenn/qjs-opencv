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
  the node space of `EDITOR.md` S3) and are mapped to the whole image:
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

## 14. Handoff: model-based cutout and tap-to-segment for collage (2026-10-29)

Audience: the implementer of `tools/opencv-imgproc.js` in qjs-opencv. Consumer: the
Wild Beauty collage editor (`EDITOR.md`, `UX-REQUIREMENTS.md` J4). This section
extends S5.1 (`cutout`, `method: "dnn"`) and adds two commands, `prepare` and
`segment`. It replaces the thin "dnn" idea of S6.3.1 with the real requirement.

### 14.1 Goal

A collage artist cuts subjects out of photos and layers them. Quality bar: a clean
subject cut with **soft, natural edges** (fur, hair, leaves), no specks, no leftover
background, no coloured halo. Plain grabCut from a rectangle is **not** good enough
(the opencv.js `removeBackground` demo is the example of what to beat).

Two modes of operation:

- **A. One-button cutout**: no user input; the main subject, or every separate
  subject, is cut out (salient-object and matting models).
- **B. Tap-to-segment**: the user taps the object (and may tap again to add or to
  subtract); the model returns that object's mask (promptable model, SAM class).

### 14.2 Output: pixels, not polylines

A polygon cannot carry a soft edge, so the **primary output is an RGBA PNG** (alpha =
the soft matte at the photo's full resolution, `trim`med to the subject). Polygons are
only a secondary, optional output (S14.5) for the editor's vector edge effects
(rim, shadow, torn edge), which need an outline. Never flatten the soft alpha into a
binary mask. Colours at the edge must be **decontaminated** (no background colour
bleeding into semi-transparent pixels): composite a result over saturated green and
magenta to check.

### 14.3 What OpenCV 5.x offers (checked in `~/Sources/opencv`, `~/Sources/opencv_contrib`)

**Version caveat.** There is no 5.1 release. `~/Sources/opencv` is the `5.x`
development branch (`5.1.0-dev`, 671 commits after the `5.0.0` tag, 2026-10-02);
`~/Sources/opencv_contrib` is 25 commits after `5.0.0`. At the `5.0.0` tag the zoo
has `u2netp`, `modnet`, `dexined` and `fcnresnet*`, but **not `sam`**: the SAM entry
and its `segmentation.py` support are development-branch only. Decide, and write down
in `capabilities`, whether the tool targets the release (5.0.0: Mode A works with
`u2netp`/`modnet`, Mode B needs a model of our own wrapped by hand) or the dev branch
(`sam` from the zoo). Whatever is chosen, name the exact commit/tag it was built from.

**Build to use (decided 2026-10-29).** `/opt/opencv-5.0.0-x86_64` (the directory name
is misleading) is already the dev branch: its libraries are `5.1.0`, built from
opencv `5c09038eb4` (`5.0.0-671`) and opencv_contrib `1275703c` (`5.0.0-25`), with
`OPENCV_ENABLE_NONFREE=ON`, and it contains `dnn`, `dnn_superres`, `alphamat`,
`ximgproc`, `xphoto`, `saliency` and `photo` (checked 2026-10-29). No rebuild is
planned; the sources in `~/Sources/opencv` are at the same commit. The SAM support in
the zoo is only a sample (`samples/dnn/segmentation.py sam`) over the ordinary `dnn`
API, so what remains to verify is whether the SAM ONNX files load and run in this
build (S14.8, question 1).

Use everything that helps; check each item by loading and running it, and report in
`capabilities`:

- **`dnn` with the new ONNX engine** (OpenCV 5.x). `samples/dnn/models.yml` is the
  model zoo with urls and sha1: `u2netp` (320x320, salient object), `modnet`
  (512x512, portrait matting, `alpha_matting.py`), `sam` (`vision_encoder.onnx` +
  `prompt_encoder_mask_decoder.onnx` from Hugging Face `onnx-community/sam-vit-base-ONNX`,
  1024x1024, `segmentation.py sam --point=x,y` shows the padding preprocessing and
  the named decoder inputs), `fcnresnet*` (semantic classes), `dexined` (edges). The
  OpenCV 4.x ONNX importer could not load most of these; the 5.x engine may (`sam`:
  development branch only, see the caveat above). **First
  task: report which of these load and run on this build** (the old S13 item).
  Candidates beyond the zoo, if they load: IS-Net / DIS, BiRefNet, MobileSAM,
  EfficientSAM, SAM 2 tiny (Hugging Face `onnx-community`).
- **contrib `alphamat`** (information-flow matting from a trimap, `samples/` has
  trimaps): refine the model's mask into a real matte in the unknown band.
- **contrib `ximgproc`**: `guidedFilter` (upscale and sharpen a low-resolution mask
  against the full-resolution photo; likely the best cheap refinement),
  `fastGlobalSmootherFilter`, `weightedMedianFilter`, superpixels (`slic`, `seeds`,
  `lsc`) for cleaning a mask along image edges.
- **`photo` / contrib `xphoto` inpaint**: fill the hole left in the original (S14.4).
- **contrib `dnn_superres`**: only if it beats `guidedFilter` for upscaling a mask
  (try it, I expect it does not). Not needed for the photos themselves.
- **contrib `saliency`**: classical spectral-residual saliency, no model. Useful only
  as a zero-dependency fallback for `auto`; I expect it to be too weak for cutouts.
- Non-free contrib algorithms are allowed, but none is needed for this.
- **Licences.** Check each model before bundling (to my knowledge: U2-Net Apache-2.0,
  SAM Apache-2.0, MODNet Apache-2.0 but portraits only, RMBG-1.4/2.0 non-commercial).
  Never bundle weights; `capabilities` lists `licence` and `commercial` per model.

### 14.4 Mode A: one-button cutout

```
opencv-imgproc.js cutout --in a.jpg --out cut.png --method dnn --dnn-model u2netp \
    [--refine none|guided|matting] [--subjects largest|all] [--split-dir DIR] \
    [--rest-out rest.png --rest-fill none|inpaint] [--min-area 0.01] \
    [--trim --trim-pad 2] [--outline-out FILE.json --outline-epsilon 1.5]
```

(Job keys are the same names, S4.) Behaviour:

1. Run the model at its input size, get a probability map, upscale it to the photo's
   full resolution with `--refine guided` (default; `matting` = trimap from the
   eroded/dilated mask, then `alphamat`, slower and better on fur).
2. `--subjects largest` keeps the biggest connected part. `--subjects all` keeps every
   part above `--min-area` (fraction of the photo); each becomes its **own** RGBA PNG,
   `cut-1.png`, `cut-2.png`, ... in `--split-dir` (`--out` receives the union).
3. `--rest-out`: the original with all cut subjects removed (alpha hole with the same
   soft edge), or with `--rest-fill inpaint` the hole filled by inpainting.
4. Everything is **trimmed** with its offset reported, so the editor can put each
   piece back exactly where it was (S14.6).

### 14.4b Mode B: tap-to-segment

A promptable model has a heavy image encoder (run once per photo) and a light decoder
(run per tap). A process per tap must not repeat the encoder, so there are two
commands and a cache:

```
opencv-imgproc.js prepare --in a.jpg --model sam [--cache-dir DIR]
opencv-imgproc.js segment --in a.jpg --model sam --point 640,410 [--point 90,300,0 ...]
    [--box x,y,w,h] [--candidates 3] [--pick best|0|1|2] [--cache-dir DIR]
    --out obj.png [--rest-out rest.png --rest-fill none|inpaint]
    [--mask-out m.png] [--refine none|guided|matting] [--trim] [--outline-out F.json]
```

- `prepare` computes the encoder output and stores it in `--cache-dir`, keyed by
  sha256(input bytes) + model name + model sha1; a second call is a no-op.
  `segment` uses the cache, and computes it first if missing (reports
  `cache: "hit"|"miss"` and the time).
- `--point x,y[,label]`: label 1 = on the object (default), 0 = not the object. Repeat
  for several. **Stateless**: the editor sends every point each time, so there is no
  session between calls. `--box` is an optional rough box. Coordinates are pixels of the
  EXIF-corrected photo; `--units norm` takes 0..1.
- SAM returns several candidate masks (part, object, whole). `--candidates 3` writes
  `obj-0.png` ... `obj-2.png` with their scores, and `--pick` chooses; the editor
  offers "bigger / smaller" to step through them.
- Latency targets (measure and report, adjust if unrealistic on the server CPU):
  `prepare` seconds, `segment` with a warm cache under about 500 ms. If ViT-B is too
  slow, try a smaller encoder (MobileSAM, EfficientSAM, SAM 2 tiny) when it loads.
- `capabilities` gains `models: [{ name, kind: "salient"|"matting"|"promptable",
  licence, commercial, loaded, unavailable, inputSize }]`.

### 14.5 Result additions

On top of S8, each output file is listed with its place in the original photo:

```json
"outputs": [
  { "role": "cut",  "path": "cut-1.png", "bbox": {"x":620,"y":410,"w":900,"h":1300},
    "offset": {"x":620,"y":410}, "coverage": 0.12, "score": 0.97 },
  { "role": "rest", "path": "rest.png", "fill": "inpaint" }
],
"outlines": [ { "output": "cut-1.png", "polygon": [[x,y],...], "epsilon": 1.5 } ],
"model": { "name": "u2netp", "licence": "Apache-2.0" },
"timings": { "readMs": 40, "prepareMs": 0, "modelMs": 900, "refineMs": 120, "writeMs": 80 }
```

`outlines` is the alpha > 127 contour per cut, simplified (Douglas-Peucker with
`epsilon`), in the coordinates of the **cut file** (so the editor draws rims and
shadows from it, and may use it as a crisp clip when a soft edge is not wanted).

### 14.6 How the editor uses it (so you know what the outputs are for)

The editor never relies on a clip path alone. For one cut it **replaces** the photo
step by two photo steps with the same matrix: the `rest` image underneath and each
`cut` image on top, shifted by its `offset` (scaled by the photo's pixel-to-unit
factor). Before the user moves anything the picture looks unchanged; moving a cut
reveals the hole (or the inpainted background). The server registers `cut` and `rest`
as derived uploads of the same owner (expiry as for uploads), with the original hash
recorded as provenance. One undo step restores the original. A rough vector clip stays
available for the cases where a crisp polygon is what the artist wants.

### 14.7 Tests and acceptance

- A small set of real photos with expected masks drawn by hand: flat background,
  cluttered scene, fur (cat on a blanket), hair, a transparent or glossy object,
  two separate subjects, a subject touching the border. Commit the list and sha1s,
  not the large images.
- Metrics: IoU against the hand mask; no halo (composite over green and magenta, the
  fringe pixel count below a threshold); `--subjects all` returns the expected number
  of parts; the original with `rest` and `cut` composited back equals the original
  (difference under a threshold where alpha = 1).
- `segment`: a point on the cat returns the cat; a second point with label 0 on the
  blanket does not change it; a point on the blanket returns the blanket.
- Report timings for a 12 MP photo on this machine, for each model and refinement.
- Optional `--preview FILE`: the result composited over a checkerboard, green and
  magenta side by side, for judging edges.

### 14.8 Open questions for the implementer

1. Which zoo and Hugging Face models load in the chosen OpenCV build (5.0.0 or the
   5.x dev branch), which not (operator names)?
2. `guided` against `matting` against `dnn_superres` for the matte: measured, on fur.
3. Is a long-lived mode (`--serve`, JSON lines on stdin and stdout, models kept in
   memory) worth it, if loading the models dominates a call?
4. Is `rest-fill inpaint` good enough, or should the hole stay transparent only?
5. Memory and time caps for 24 MP inputs (`maxPixels` stays 64 MP, S5).

## 15. As implemented: deviations from this spec (tool v1.2.0, 2026-10-28)

Where this section and an earlier one disagree, this section is what the tool
does. `tools/opencv-imgproc.js` in qjs-opencv; its tests are
`tests/unittests/test_opencv_imgproc_tool.js` (13 tests, the model ones skip
when the weights are missing).

### 15.1 Output, all commands

- **Every stdout line is NDJSON**, also `--help` and every error. Each line has
  an `event`: `progress`, `log` or `result`. The last line is always the
  `result` event (`ok: true` with the fields of S8/S14.5, or `ok: false` with
  `error: {code, message}`); stderr stays empty. Exit codes are those of S9.
- **`--progress`** (any command) prints `{event:"progress", stage, message,
  percent}` at the checkpoints. `percent` never goes back and ends at 100 with
  `stage: "done"`. Slow stages report sub-steps (strips of a big photo).
- **PNG/WebP:** `--png 0..9` sets the compression level, `--webp lossless|1..100`
  the WebP quality. (qjs-opencv `imwrite`/`imencode` take `{compression}` and
  `{quality}` options now; the 3rd argument for a palette is `{palette}`.)

### 15.2 `cutout`

- **Only `method: "dnn"`** is offered (`capabilities.methods == ["dnn"]`).
  grabCut, flood, chroma, threshold, kmeans, watershed, contour, diff, mask and
  auto were dropped: the cut quality of Mode A needs a trained matte, and
  `--method grabcut` is E_METHOD (exit 9). The S5/S6 tables above describe the
  original plan; the options of S14.4 are what exists.
- **Models:** `--dnn-model u2netp` (salient, 320x320, ~0.2 s). The model table is
  inside the tool (`name`, files, sha1, licence), not a `<name>.json` sidecar
  (S6.3.1 is not used); `capabilities.models[]` lists each with `loaded`.
- **New options:**

  | Option | Meaning |
  |---|---|
  | `--refine none\|guided\|matting` | matte refinement: ximgproc guided filter (default, radius 0.6% of the long edge, then a contrast steepening) or contrib `alphamat.infoFlow` on a trimap. Matting took 13 s (guided 1 s) and looked the same on the duck and on a long-hair cat: use `guided`. |
  | `--edge-key auto\|none\|#rrggbb`, `--edge-strength 0..1`, `--edge-band N` | edge-only colour-to-alpha cleanup (`defringe`): inside a narrow band at the cut edge, pixels that match the local background (estimated, or the key colour) lose alpha. Removes dark/light rims. |
  | `--rest-fill none\|inpaint\|fsr` | the rest image: `none` (default) leaves the hole transparent (alpha = 1 - subject union, soft), `inpaint` fills it with Telea at work size 1024 (0.5 s), `fsr` with xphoto frequency-selective reconstruction at work size 512 (13 s). Large holes look smeared with either. |
  | `--preview FILE` | writes a checkerboard preview of the cut |

- Foreground colours are estimated (blur fusion, Germer 2020), so the RGB of
  edge pixels is decontaminated, not just the alpha.
- Two subjects that touch are one connected component, so `--subjects all`
  gives them as one subject; split them with `segment` taps.
- Memory: the heavy float stages run strip-wise; a 12 MP photo takes 5.3 s and
  1.1 GB, a 1 MP photo about 1 s and 0.4 GB.

### 15.3 `prepare` and `segment` (Mode B)

- **Models:** `--model mobilesam` (default) or `sam`.

  | | encoder + decoder | cold `prepare` | RSS | cache file |
  |---|---|---|---|---|
  | `mobilesam` | 27 MB + 16 MB (Acly/MobileSAM onnx export; weights Apache-2.0, export MIT) | 1.3-2.4 s | 0.5 GB | 4 MB |
  | `sam` (ViT-B) | 343 MB + 16 MB (HF export, Apache-2.0) | 8 s | 7.5 GB | 8 MB |

  The two give masks of comparable quality on the test photos (duck in a hand,
  flower, cats, a grey long-hair cat); `sam` is kept for photos where
  mobilesam fails. Warm `prepare` is a 0.3 s no-op.
- **Cache:** `--cache-dir` (env `WB_CACHE_DIR`, else `<repo>/cache`), key
  `<sha256 of the input bytes>-<model>-<first 12 of the encoder sha1>.emb` plus
  a `.json` meta file. A different model or weights file is a different key.
- **Prompts:** `--point x,y[,label]` (repeatable, label 1 positive, 0
  negative), `--units px|norm`. The decoders have **no box input**: `--box
  x,y,w,h` is emulated by a positive tap at the box centre, then the candidate
  whose extent overlaps the box best is picked (`boxIou` in the result).
- **Candidates:** `--candidates 1..3` writes `<out>-0`, `-1`, `-2` next to
  `--out`; `--out` itself is the picked one (`--pick best|0|1|2`). One tap gives
  sub-part, part and whole subject. **The model's own score is not a quality
  ranking** (a partial mask can outscore the whole subject), so an editor should
  show the candidates and let the user choose; `best` is only the highest score.
- **Latency:** `--preview-size N` runs the whole cut on a photo reduced to N px
  (all coordinates in the result are then in those pixels, `scale` and
  `original` say how to map back): 1024 on a 12 MP photo takes 0.8 s and 240 MB.
  Warm `segment` at full 12 MP is 5.8 s for one candidate. For taps use the two
  phases of S15.3b: a preview line first, the full-size line after.
- The result of `segment` has the fields of S14.5 plus `cache` (`hit|miss`),
  `model`, `refine`, `candidates[]` (`index`, `score`, `picked`, `path`,
  `bbox`), `picked`, `best`, `prompts`, `scale`, `original`, `timings`.

### 15.3b Two phases: `--view-size`, `--phases` (tool v1.3.0)

For interactive taps. No resident worker: every call is one process, killing it
at any moment is safe (see below).

```
opencv-imgproc segment --in photo.jpg --out wand.png --point 0.5,0.5 --units norm \
    --cache-dir D --view-size 1024 --phases preview,final --refine none --candidates 1
opencv-imgproc cutout  --in photo.jpg --out cut.png --view-size 1024 [--progress]
```

- `--view-size N` (long edge in px, default 1024 when `--phases` is given
  without it) alone means `--phases preview,final`. `--phases` is `preview`,
  `final` or `preview,final` (anything else: E_USAGE, exit 2). Without either
  option there is one result line, as before. (`--preview-size N` without
  `--phases` keeps its old meaning: one result at that size.)
- Phase `preview` runs everything after the model (soft matte, refine, subjects,
  edge, foreground colour, files) on a copy of the photo reduced to
  `--view-size` and prints its result **at once** (every stdout line is
  flushed). Phase `final` then runs the same pipeline at full size from the same
  model output (`segment`: the same 256x256 candidate logits, so the decoder is
  not run twice; `cutout`: the same u2netp matte) and prints the **last** line.
  If the photo is not larger than `--view-size`, both phases are the same size.
- Warm (embedding cache hit, mobilesam), 12 MP photo: the preview line arrives
  0.6-0.7 s after the tool starts (0.87-0.94 s wall including process start),
  the final line after about 5.5 s (`--refine none` or `guided` alike);
  `segment` on a 1.2 MP photo: preview 0.8 s, final 1.15 s wall. `cutout`
  preview on 12 MP: 1.2 s wall (the u2netp model load and run, 0.5 s, comes
  first). `--cache-dir` must be the one `prepare` used (or the first
  `segment` call fills it).
- Each result line is a normal result (S8/S14.5) plus:

  | Key | Meaning |
  |---|---|
  | `phase` | `"preview"` or `"final"` (also on an error result of a phased run) |
  | `scale` | tier size / photo size (1 for final) |
  | `original` | `{width, height}` of the photo |
  | `view` | `{width, height}` of this tier's image (= the output size without `--trim`) |
  | `bbox`, `offset`, `output.width/height` | in the pixels of this tier |
  | `bboxNorm` | `bbox` as 0..1 of `view`, the same for both tiers |
  | `outlines[]` | `{output, polygon, epsilon}` in the pixels of that output file (`output.width` x `output.height`); always present now, `--outline-out` only also writes them to a file |
  | `timings.phaseMs` | time of this phase; `totalMs` counts from tool start |

- Files: preview `<out stem>.preview.<ext>` (candidates `<stem>.preview-N.<ext>`,
  `--mask-out` and `--outline-out` likewise `<name>.preview.<ext>`); final the
  names you gave. `--rest-out`, `--preview` and `--split-dir` are final only.
  The preview files stay on disk; the caller deletes the out dir.
- `--progress`: events carry `phase`; `percent` runs 0-30 in `preview` and
  30-100 in `final` and never goes back.
- Kill safety: every output (images, json, cache files) is written to a temp name
  `<file>.tmp-<pid>` and renamed, so a file under its final name is always
  complete. A killed run can leave an orphan `.tmp-<pid>` (the cache dir sweeps
  those older than 10 minutes). A run killed after the preview line leaves the
  complete preview files and no final file.
- `prepare` is unchanged (one result line; the cache survives on disk, atomic as
  above): call it at upload time, `segment` then hits the cache.
- Output: one result line per phase.

  Example (the `segment` command above on a 3024x4032 photo, paths shortened, polygon cut):

      {"event":"result","phase":"preview","ok":true,"command":"segment","scale":0.254,"original":{"width":3024,"height":4032},"view":{"width":768,"height":1024},"output":{"path":"wand.preview.png","format":"png","width":768,"height":1024,"hasAlpha":true},"bbox":{"x":0,"y":0,"w":768,"h":1024},"bboxNorm":{"x":0,"y":0,"w":1,"h":1},"offset":{"x":0,"y":0},"outlines":[{"output":"wand.preview.png","polygon":[[536,404],[522,404],[510,407],"..."],"epsilon":1.5}],"picked":1,"cache":"hit","model":"mobilesam","timings":{"prepareMs":16,"decodeMs":242,"subjectsMs":69,"colourMs":81,"writeMs":20,"totalMs":614,"phaseMs":191}}
      {"event":"result","phase":"final","ok":true,"command":"segment","scale":1,"original":{"width":3024,"height":4032},"view":{"width":3024,"height":4032},"output":{"path":"wand.png","format":"png","width":3024,"height":4032,"hasAlpha":true},"bbox":{"x":0,"y":0,"w":3024,"h":4032},"bboxNorm":{"x":0,"y":0,"w":1,"h":1},"offset":{"x":0,"y":0},"outlines":[{"output":"wand.png","polygon":[[2112,1591],[2056,1591],[2053,1594],"..."],"epsilon":1.5}],"picked":1,"cache":"hit","model":"mobilesam","timings":{"prepareMs":16,"decodeMs":242,"subjectsMs":2373,"colourMs":1599,"writeMs":513,"totalMs":5149,"phaseMs":4535}}

### 15.3c Directories of an installed copy

A checkout (`tools/` beside `CMakeLists.txt`) uses `<repo>/models` and
`<repo>/cache`. An installed copy (`<prefix>/bin/opencv-imgproc`) looks for the
models in `$XDG_DATA_HOME/opencv-imgproc/models` (default
`~/.local/share/opencv-imgproc/models`), then `/usr/local/share/opencv-imgproc/models`,
then `/usr/share/opencv-imgproc/models`, and caches in
`$XDG_CACHE_HOME/opencv-imgproc` (default `~/.cache/opencv-imgproc`).
`--models-dir` / `WB_MODELS_DIR` and `--cache-dir` / `WB_CACHE_DIR` always win.
The weights are never installed by the tool. A missing file is E_MODEL with
`detail.modelsDir` naming the directory that was searched.

### 15.4 Bindings used (qjs-opencv)

`cv.ximgproc.guidedFilter` (not `cv.guidedFilter`), `cv.alphamat.infoFlow`,
`cv.xphoto.inpaint`, `cv.dnn` `Net` with named inputs and `forward([names])`
returning an array, `cv.hash.sha1/sha256` (cache keys, model checks),
`cv.setRNGSeed`. `Mat.setTo(Scalar)` follows opencv.js. A third `imwrite` /
`imencode` argument is a params array or an options object (`palette`,
`transparent`, `compression`, `quality`, `params`, `delay`, `loop`).

### 15.5 Checked, and not yet

Checked on real photos: a duckling held in a hand, a 12 MP flower, a grey
long-hair cat (fur, whiskers), two cats side by side, and a 12 MP photo of a
can, a paper bag and a pastry on a bench (`cutout --subjects all --split-dir`
gives the three as separate files in 7.5 s). Not checked: a subject touching
the image border, the composite-back test of S14.7 on a coloured backdrop and
hair on skin.
Known weakness: the u2netp matte also takes dark parts of the scene that
bridge to a subject (the can got a dark tail from the gap behind it, the pastry
a grey strip above it); the `segment` masks of the same items are clean, so
use taps when the backdrop has dark gaps. Thin fur or hair strands over a blurred background keep some
background colour on their edge pixels with both `guided` and `matting`; a
tap-based `segment` mask is cleaner there than the u2netp matte.

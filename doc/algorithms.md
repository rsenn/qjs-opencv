# `cv.*` algorithms (custom)

Functions exported by `js_algorithms.cpp`. These are *not* OpenCV functions — they live under `algorithms/` in this repo and are surfaced on the `cv` module alongside the OpenCV-proper bindings.

Two thematic groups:

| Group | Functions | Pipeline |
|---|---|---|
| Skeleton / line extraction | `skeletonization`, `pixelNeighborhood`, `pixelNeighborhoodCross`, `pixelFindValue`, `traceSkeleton` | binary mask → 1-px skeleton → degree map → ordered polylines |
| Palette reduction | `paletteGenerate`, `paletteApply`, `paletteMatch` | image → dominant colors → indexed image → recolored image |

All functions are static methods on the `cv` namespace:

```js
import { skeletonization, pixelNeighborhood, traceSkeleton,
         paletteGenerate, paletteApply, paletteMatch,
         Mat, CV_8UC1 } from 'opencv';
```

---

## `skeletonization(src, dst)`

Zhang–Suen thinning of a binary image to a 1-pixel-wide skeleton.

- **`src`** — `cv.Mat` or `cv.UMat`. Any number of channels; multi-channel input is converted to grayscale internally.
- **`dst`** — `cv.Mat` or `cv.UMat`. Receives a `CV_8UC1` image with foreground pixels at 255 and background at 0.
- **Returns** — `undefined`.

### Behavior

1. Source is converted to grayscale (`COLOR_BGR2GRAY`) when it has more than one channel.
2. Otsu thresholding (`THRESH_BINARY + THRESH_OTSU`) produces a 0/255 binary mask.
3. Zhang–Suen thinning iterates two sub-passes (NE-peel, SW-peel) until no pixel changes between iterations. The result is rotation-symmetric and topology-preserving.
4. Output is rescaled to 0/255.

Source is not modified — the algorithm copies into an internal buffer first.

### Notes

- Border pixels (`y == 0`, `y == rows-1`, same for x) are never thinned because the kernel needs all 8 neighbours. A skeleton touching the image edge will retain a small "stub" outside the thinned region.
- Cost is dominated by `cv::Mat::at<uchar>` random access inside the inner loop and one `cv::Mat` allocation per sub-iteration. Acceptable for offline work; allocate-and-reuse before using in a real-time pipeline.

### Example

```js
import { Mat, COLOR_BGR2GRAY, cvtColor, skeletonization } from 'opencv';
const skel = new Mat();
skeletonization(input, skel);   // input can be color or grayscale
```

---

## `pixelNeighborhood(src, dst)`

8-neighbour foreground count for each foreground pixel. Operates on a binary `CV_8UC1` mask (typically a skeleton). The output is a *degree image*: for every non-zero pixel in `src`, `dst` holds the count of non-zero neighbours among `{NW, N, NE, W, E, SW, S, SE}`.

- **`src`** — `cv.Mat` only (not UMat). `CV_8UC1`, continuous.
- **`dst`** — `cv.Mat` or `cv.UMat`. Replaced with a `CV_8UC1` image of the same size.
- **Returns** — `undefined`.

Output values for skeleton pixels:

| value | meaning |
|---|---|
| 0 | isolated pixel |
| 1 | endpoint of a polyline |
| 2 | mid-curve pixel |
| ≥ 3 | branch / junction |

### Notes

- `dst` is **not zero-initialized** at non-foreground pixels — those cells contain garbage. Downstream `traceSkeleton` only consults the map at foreground positions, so this is safe in the intended pipeline; if you visualize the map directly, mask with `src` first.
- Border rows/columns (`y < 1` or `y > rows - 3`, same for x) are skipped.

### Example

```js
import { Mat, pixelNeighborhood } from 'opencv';
const deg = new Mat();
pixelNeighborhood(skel, deg);
```

---

## `pixelNeighborhoodCross(src, dst)`

Identical to `pixelNeighborhood` except the kernel is 4-connected (N, W, E, S only). Output values are in `[0, 4]`.

Use this when you want to ignore diagonal connections — e.g. for grids where diagonals are not considered adjacency.

```js
pixelNeighborhoodCross(skel, deg4);
```

---

## `pixelFindValue(src, value)`

Returns the coordinates of every pixel whose value equals `value`.

- **`src`** — `cv.Mat`, `CV_8UC1`.
- **`value`** — non-negative integer; values ≥ 256 are silently truncated to `uchar`.
- **Returns** — array of `cv.Point` (integer coordinates).

Useful as a seed picker after `pixelNeighborhood`: `pixelFindValue(deg, 1)` enumerates all skeleton endpoints; `pixelFindValue(deg, 3)` enumerates 3-way junctions.

### Example

```js
import { pixelNeighborhood, pixelFindValue } from 'opencv';
pixelNeighborhood(skel, deg);
const endpoints = pixelFindValue(deg, 1);
const junctions = pixelFindValue(deg, 3).concat(pixelFindValue(deg, 4));
```

---

## `traceSkeleton(src [, contoursOut [, neighborhood [, mapping]]])`

Extracts ordered polylines from a 1-pixel-wide skeleton. The result is a set of `cv.Contour`s suitable for downstream `approxPolyDP`, `drawContours`, or SVG export.

### Signatures

```js
const contours = traceSkeleton(skel);                          // (1)
const count    = traceSkeleton(skel, contoursArr);             // (2)
const count    = traceSkeleton(skel, contoursArr, neighborhood);
const count    = traceSkeleton(skel, contoursArr, neighborhood, mapping);
```

- **`src`** — `cv.Mat`. The skeleton mask (output of `skeletonization`). Not modified by the call.
- **`contoursOut`** *(optional)* — an existing JS array; contours are pushed into it.
- **`neighborhood`** *(optional)* — `cv.Mat` that receives the internal degree map (same as `pixelNeighborhood` would produce).
- **`mapping`** *(optional)* — `cv.Mat`, `CV_32SC1`, that receives a per-pixel label image where each pixel holds the index of the contour that consumed it (or `-1` if untouched). Useful for debugging or for re-coloring the skeleton by contour.

Return:
- form (1): a new array of `cv.Contour`.
- forms (2)–(4): the number of contours written.

### Behavior

Greedy directional walk:

1. Build the 8-neighbour degree map (`pixelNeighborhood`) and a label image (all −1).
2. Scan in row-major order for an unclaimed pixel whose degree is > 0.
3. Walk: at each step, prefer the same step direction as last time, fall back to the step-before-that, then to any unclaimed neighbour in a narrow forward fan. Stop when no candidate is available.
4. Each consumed pixel decrements the degree of its 8 neighbours, which makes already-used parts of the skeleton invisible to the seeding scan.
5. Repeat from step 2 until no seed remains.

Coordinates inside each returned contour are `double` so they are directly compatible with the rest of the `cv.Contour` API.

### Notes / caveats

- **`simplify=true` is hard-coded.** Strictly collinear runs of pixels are collapsed to their endpoints (RDP-lite). If you need every pixel — e.g. for animation along the curve — wrap the call in C++ or modify the binding.
- **Junction tie-breaking is row-major.** At a Y-junction, which arm becomes the "main" contour and which gets emitted as a separate contour depends on which one the row-major seed scan reaches first. Deterministic, but not topology-aware.
- The four-argument form is the inspection mode — pass `neighborhood` and `mapping` to see what the tracer saw.

### Example

```js
import { Mat, skeletonization, traceSkeleton } from 'opencv';

const skel = new Mat();
skeletonization(input, skel);

const contours = traceSkeleton(skel);
console.log(`${contours.length} polylines, ${contours.reduce((n, c) => n + c.length, 0)} points`);
```

---

## `paletteGenerate(src [, mode [, count]])`

Returns the dominant colors of an image as a JS array of `[B, G, R]` triplets.

- **`src`** — `cv.Mat`. Color image.
- **`mode`** *(optional, default 0)* — packed flags:
  - bit 0: color space (`0` = BGR, `1` = HSV).
  - bits 1–2: distance metric (`0` = CUBE, `1` = CIE76, `2` = CIE94, `3` = K-means).
- **`count`** *(optional, default 0 → 12)* — number of colors to extract. `0` uses the implementation default of 12.
- **Returns** — array of `[B, G, R]` triplets (`Number[3]`), each component in `0..255`.

Backed by `dominant_colors_grabber::GetDomColors`. The default mode (`0`) is BGR cube-distance, which matches the rest of OpenCV's color conventions.

### Example

```js
import { imread, paletteGenerate } from 'opencv';
const img = imread('photo.jpg');
const colors = paletteGenerate(img, 0, 8);   // 8 dominant BGR colors
```

---

## `paletteApply(src, dst, palette)`

Recolors an indexed (single-channel) image by looking each pixel value up in the supplied palette.

- **`src`** — `cv.Mat`, `CV_8UC1`. Pixel values are treated as palette indices (0..255).
- **`dst`** — `cv.Mat` or `cv.UMat`. **The channel count of `dst` selects the output format**:
  - 1-channel `dst` is treated as a request for 3-channel BGR output.
  - 3-channel `dst` → BGR; palette is interpreted as RGBA from JS and the R/B channels are swapped on the way in.
  - 4-channel `dst` → BGRA; palette is used as-is (no swap).
  - Any other channel count throws.
- **`palette`** — JS array of `[R, G, B, A]` quadruplets (alpha optional). Up to 256 entries.

### Notes

- Because of the swap inconsistency between the 3- and 4-channel branches, colors will not match exactly between a BGR output and a BGRA output rendered from the same palette. Decide on a target channel count first and stick with it.
- The output is rebuilt as `CV_8UC3` internally regardless of `dst`'s channel count, then copied into `dst`.

### Example

```js
import { paletteGenerate, paletteApply, Mat, CV_8UC3 } from 'opencv';
const palette = paletteGenerate(img, 0, 16);          // 16 dominant BGR colors
// quantize img to a 1-channel index image first (e.g. via paletteMatch), then:
const recolored = new Mat(img.size(), CV_8UC3);
paletteApply(indexImage, recolored, palette);
```

---

## `paletteMatch(src, dst, palette)`

Quantizes a color image by replacing each pixel with the index of the nearest palette color.

- **`src`** — `cv.Mat`. Color image (3- or 4-channel `CV_8U`).
- **`dst`** — `cv.Mat` or `cv.UMat`. Receives a `CV_8UC1` index image: each pixel is the position in `palette` of its nearest match.
- **`palette`** — JS array of `[R, G, B, A]` quadruplets.

Distance is computed in a perceptually weighted RGB space (the "low-cost approximation" from <https://www.compuphase.com/cmetric.htm>), not plain Euclidean. The metric is:

```
ΔE² = (2 + r̄/256) · ΔR² + 4 · ΔG² + (2 + (255 - r̄)/256) · ΔB²
```

where `r̄` is the mean red of the two colors (all components normalized to `[0,1]`). Green is weighted ~4× and red/blue tilt with the mean red value, giving green-heavy and skin-tone differences slightly more weight than a flat Euclidean distance would.

### Notes

- Pairs naturally with `paletteApply`: `paletteMatch` produces the index image, `paletteApply` recolors it. Together they implement a "quantize then recolor with a different palette" operation.
- For RGBA palettes with one transparent slot, the underlying C++ supports a `transparent` index; this is not currently exposed to JS (the binding always calls with the no-transparent branch).

### Example

```js
import { paletteGenerate, paletteMatch, paletteApply, Mat, CV_8UC3 } from 'opencv';

const palette  = paletteGenerate(img, 0, 16);
const indexed  = new Mat();
paletteMatch(img, indexed, palette);                  // 16-color quantization

const recolored = new Mat(img.size(), CV_8UC3);
paletteApply(indexed, recolored, palette);            // back to BGR
```

---

## End-to-end pipeline

The skeleton-tracing functions are designed to be chained:

```js
import { Mat, COLOR_BGR2GRAY, cvtColor,
         skeletonization, pixelNeighborhood, traceSkeleton } from 'opencv';

const gray = new Mat();
cvtColor(input, gray, COLOR_BGR2GRAY);

const skel = new Mat();
skeletonization(gray, skel);                    // stage 1

const deg = new Mat();
pixelNeighborhood(skel, deg);                   // stage 2 — optional inspection

const contours = traceSkeleton(skel);           // stage 3
// contours: Array<cv.Contour> with double-precision points
```

For palette reduction:

```js
import { paletteGenerate, paletteMatch, paletteApply, Mat, CV_8UC3 } from 'opencv';

const palette = paletteGenerate(input, 0, 8);
const indexed = new Mat();
paletteMatch(input, indexed, palette);

const out = new Mat(input.size(), CV_8UC3);
paletteApply(indexed, out, palette);
```

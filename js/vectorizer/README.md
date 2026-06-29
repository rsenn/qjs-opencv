# qjs-vectorizer

A suite of QuickJS scripts (rsenn's `qjs-opencv` / `qjs-modules`) that extract
**vector data** from images and video frames using many different methods, lets
you tune each one in a HighGUI wizard, and composes the results into a single
**SVG** — with per-frame affine *and* perspective placement.

```
qjsm vectorizer.js [-o out.svg] [-m canny] [-W 1280] [-H 720] <image|video|dir>...
```

Four-stage wizard:

1. **Load** — every CLI path (file or directory) becomes a thumbnail. Click
   images to toggle them into the frame set; click a video to scrub and add
   individual frames.
2. **Assign** — give each frame a vectorization method (left = frames, right =
   method palette, plus *apply-to-all*).
3. **Process** — trackbars built live from the method's parameter schema, with a
   split *source | vectorized* preview.
4. **Project** — drag 4 corner handles per frame to place/warp it onto the
   output canvas; composite preview mirrors the exported SVG exactly.

`Export` (or the toolbar button) writes the SVG. `TAB` advances, `ESC` quits.

---

## Is MVC appropriate? — yes, with a twist

The brief asks for "proper GUI design patterns" and specifically about MVC. MVC
is the right backbone here, refined into three cooperating patterns:

* **Model** (`core/model.js`) — one shared, observable document holding the
  whole pipeline state (sources, frames, assignments, results, placements,
  canvas). It extends a tiny `Emitter` and emits change events. Crucially it
  **never imports OpenCV**; frame Mats are stored as opaque handles. That keeps
  the model layer pure and testable.
* **View + Controller per stage** (`gui/stages/stage{1..4}-*.js`) — each wizard
  step is its own immediate-mode View that also handles its local interaction.
  There is no retained widget tree: each frame the View is re-derived from the
  Model (`draw(app)`), which makes the MVC loop trivial to reason about.
* **Controller / state machine** (`gui/app.js`) — owns the single HighGUI
  window, the input snapshot, the shared HUD and the trackbar manager, and
  sequences the four stages (`goTo/next/back`, gated by `pipeline.canAdvance`).

Two more patterns do the heavy lifting:

* **Strategy / Plugin** for vectorization methods (`vector/base.js` +
  `vector/methods/*` + `vector/registry.js`). Each method is a self-contained
  class file; adding one touches nothing but the registry.
* **Pipeline** (`core/pipeline.js`) — the generic LOAD→ASSIGN→PROCESS→PROJECT
  abstraction, with all concrete work injected (`loader`, `registry`,
  `composer`). You could retarget it to "SVG from PDF pages" by swapping the
  loader, no GUI changes.

### The decoupling boundary (the part the brief stresses)

```
core/      pure JS, ZERO OpenCV     model, pipeline, geometry, vectordata, svg, composer
vector/    Strategy plugins         base + registry + methods/* (OpenCV only inside apply())
cv/        OpenCV adapters          loader (frames), convert (cv→VectorData), raster (preview)
gui/       OpenCV only for HighGUI  app, canvas, widgets, mouse, trackbars, stages/*
```

The contract that glues them is **`VectorData`** (`core/vectordata.js`): a
backend-agnostic shape list (`polyline/polygon/path/line/circle/ellipse/point`
+ style). Methods *produce* it, the projector *transforms* it, the SVG composer
*consumes* it — none of them knows which method ran. The generic 4-step pipeline
therefore has no OpenCV in it at all; `cv.Mat`/`cv.Rect`/projection matrices are
confined to `cv/` and `gui/`.

---

## Vectorization methods (pluggable)

Ten ship in the default registry; the first is the required **Canny → contours**:

| id          | label                | idea                                            |
|-------------|----------------------|-------------------------------------------------|
| `canny`     | Canny + Contours     | Canny edges → `findContours` → stroked polylines (line art) |
| `threshold` | Threshold Regions    | (adaptive) threshold → filled contours with holes |
| `palette`   | Palette Regions      | posterize/quantize → "paint-by-numbers" color regions |
| `lowpoly`   | Low-Poly Delaunay    | `goodFeaturesToTrack` + `Subdiv2D` triangulation |
| `hough`     | Hough Lines          | probabilistic Hough line segments               |
| `lsd`       | LSD Lines            | `LineSegmentDetector` (NONFREE build)           |
| `fastline`  | Fast Line Detector   | `FastLineDetector` (ximgproc)                   |
| `skeleton`  | Skeleton             | morphological skeleton centerlines              |
| `shapefit`  | Shape Fit            | fit ellipses / min-area rects / circles to contours |
| `stipple`   | Stipple              | intensity-driven dot stippling                  |

### Adding a method

Drop a file in `vector/methods/`, extend `VectorMethod`, register it:

```js
import { VectorMethod } from '../base.js';
import { create, polyline } from '../../core/vectordata.js';
import { toGray, release } from '../../cv/convert.js';
import { Mat, Canny } from 'opencv.so';

export class MyMethod extends VectorMethod {
  static id = 'mine';
  static label = 'My Method';
  static description = 'what it does';

  paramsSpec() {
    return [{ key: 'thresh', label: 'Threshold', type: 'int', min: 0, max: 255, step: 1, default: 128 }];
  }

  apply(mat, p, meta) {            // the ONLY place OpenCV runs
    const gray = toGray(mat);
    // ... produce shapes ...
    release(gray);
    return create(meta.width, meta.height, { shapes: [ /* VectorData shapes */ ] });
  }
}
```

then add `.register(MyMethod)` in `vector/registry.js`. Stage 3 turns
`paramsSpec()` into trackbars automatically — no GUI code to write.

`paramsSpec` types: `int`, `float` (mapped to integer trackbars via `step`),
`bool` (0/1), `enum` (0..N over `options`).

---

## Projection / warp (stage 4)

All projective math is **pure JS** in `core/geometry.js` (3×3 row-major
homographies; `getPerspectiveTransform` via an 8×8 Gaussian solve — the
decoupled equivalent of `cv.getPerspectiveTransform`). Dragging a frame's 4
corners builds `H = rectToQuad(frameW, frameH, quad)` and stores it on the
Model.

The SVG composer (`core/svg.js`) then:

* emits **affine** placements as a native `<g transform="matrix(...)">` (stays
  editable in Inkscape/Illustrator), and
* **bakes perspective** placements point-by-point, because SVG transforms can't
  represent a homography. Circles/ellipses are sampled to polygons under
  perspective.

The HighGUI composite preview rasterizes through the *same* `flatten()` +
`apply(H)` path (`cv/raster.js`), so **what you see is what you export**.

---

## Binding assumptions to confirm

This targets your `qjs-opencv` fork; a few call shapes are assumed and isolated
so they're easy to fix if your bindings differ:

* `findContours(img, mode, method)` returns `[contours, hierarchy]`; iterating a
  contour yields `Point`s (handled defensively in `cv/convert.js: pointsOf`).
* `LineSegmentDetector` / `FastLineDetector` expose `.detect(gray, linesMat)`;
  LSD needs a NONFREE build, FastLine needs ximgproc. (`vector/methods/lsd.js`,
  `fast-line.js`)
* `skeletonization(src, dst)` exists (ximgproc/thinning). (`skeleton.js`)
* `Subdiv2D` with `.insert(Point)` / `.getTriangleList()`. (`lowpoly-delaunay.js`)
* `drawPolygon(mat, points, scalar, thickness)` for filled preview polygons
  (falls back to stroked edges if absent). (`cv/raster.js`)
* Palette helpers `paletteGenerate(src,k)` / `paletteMatch(src,palette)` — these
  are rsenn-specific; if named differently, only `vector/methods/palette-regions.js`
  needs editing.
* Color sampling uses `mean(src, mask)` and ROI `mean(src(Rect))` rather than
  per-pixel access. (`cv/convert.js`)

House style followed throughout: **named imports** from `'opencv.so'` (no `cv.`
prefix, no star-imports), `console.log`, `fs` from `qjs-modules` with `*Sync`,
all entry logic in `main()`, `writeFileSync` over async.

---

## Layout

```
vectorizer.js                  entry point (CLI parse, wiring, run)
core/   events.js model.js pipeline.js geometry.js vectordata.js svg.js composer.js
vector/ base.js registry.js  methods/{canny-contours,threshold-contours,palette-regions,
        lowpoly-delaunay,hough-lines,lsd,fast-line,skeleton,shape-fit,stipple}.js
cv/     loader.js convert.js raster.js
gui/    app.js canvas.js widgets.js mouse.js trackbars.js  stages/stage{1..4}-*.js
```

> Note: the GUI can't be exercised in a headless environment (it needs a HighGUI
> backend and your `qjs-opencv` build). The pure-`core/` layer is runnable under
> plain Node — the projection math and SVG composition were validated that way.

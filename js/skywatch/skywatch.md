# skywatch — contrail/aircraft correlation project

## Goal

Run on a Raspberry Pi 4 with a zenith-mounted fisheye camera and an ADS-B
receiver (readsb/tar1090 + RTL-SDR). For every trail detected in the sky,
correlate it against real ADS-B aircraft tracks so each trail is either:

- **matched** to a specific aircraft (ICAO24, callsign, altitude, speed) —
  documented proof it's a normal airliner contrail, or
- **unmatched** — logged as-is, no aircraft on ADS-B explains it.

A second goal: track how a matched trail's pixel width grows over
subsequent frames, correlated with the aircraft's altitude/speed, as a
basic photogrammetric contrail-persistence measurement.

## Hardware / target environment

- Raspberry Pi 4, fisheye camera (raspi cam), zenith-mounted (straight up).
- RTL-SDR stick running **readsb** (dump1090-family), exposing aircraft
  state as `aircraft.json` (tar1090-compatible schema: `hex`, `flight`,
  `lat`, `lon`, `alt_baro`, `track`, `gs`, ...), polled over HTTP.
- qjs-opencv **not yet built on the Pi** — needs a fresh build there
  before any of the camera/CV phases can run. Geometry-only code (phase 1)
  has no OpenCV dependency and can be dev/tested off-Pi, as done here.

## Status

| Phase | What | Status |
|---|---|---|
| 1 | Geometry: ADS-B lat/lon/alt → az/el from camera | **done** — `geometry.js` |
| 1 | Fisheye projection: az/el ↔ pixel (equidistant model) | **done** — `fisheye-projection.js` |
| 1 | Unit tests for both (tinytest, run off-Pi with plain `qjsm`) | **done** — `tests/unittests/test_skywatch_geometry.js` |
| 2 | Camera calibration (`cv.fisheye.calibrate`) → real fx/cx/cy | not started — needs Pi build + checkerboard + physical camera |
| 2 | North-offset alignment (compass or celestial reference) | not started — needs physical camera |
| 3 | readsb `aircraft.json` polling loop | not started |
| 3 | Trail detection, proved on real (non-fisheye) photos | **partial** — works well on clear-sky frames; unreliable when natural cirrus is present, see phase 3 below — `examples/skywatch_contrail_detect.js` |
| 4 | Match predicted pixel (per aircraft, per frame) against detected line segments | not started |
| 4 | Match/unmatched logging format | not started |
| 5 | Width-over-time tracking per matched trail | not started |
| 5 | Correlate width growth with altitude/speed/weather (optional) | not started |

## Phase 1 — geometry (done)

`js/skywatch/geometry.js`:
- `groundDistance(lat1, lon1, lat2, lon2)` — haversine great-circle distance, meters.
- `bearing(lat1, lon1, lat2, lon2)` — initial bearing, degrees, 0=N/90=E.
- `azimuthElevation(camera, aircraft)` — `{lat, lon, altM}` for both → `{azimuth, elevation}` degrees.

`js/skywatch/fisheye-projection.js`:
- Equidistant fisheye model (`r = fx * zenithAngle`), matching the
  standard consumer/raspi fisheye lens response at the image center.
- `azElToPixel({azimuth, elevation}, calib)` and `pixelToAzEl({x,y}, calib)`,
  where `calib = {cx, cy, fx, northOffsetDeg, clockwise}`.
- `clockwise` exists because looking *up* through a lens flips handedness
  vs. looking down at a map — whether azimuth increases clockwise or
  counter-clockwise in the image depends on the physical mount and must
  be determined empirically once the camera is up (e.g. point a known
  bearing — sun position, compass-sighted landmark — at the lens and see
  which way it lands in the image).

Verified with round-trip tests and known-geometry sanity checks (aircraft
directly overhead → elevation 90°, etc.) in
`tests/unittests/test_skywatch_geometry.js`, run 5× to rule out flakiness
(pure math, deterministic, all green).

## Phase 2 — camera calibration (needs Pi + physical camera)

1. Build qjs-opencv on the Pi (`cfg.sh` release build — see repo root
   `CLAUDE.md`).
2. Capture a fisheye checkerboard calibration set, run
   `cv.fisheye.calibrate` (`js_fisheye.cpp`/`js_calib3d.cpp`) to get real
   `fx, cx, cy` — the intrinsics used for `fisheye-projection.js`'s `calib`
   object. Note: `cv.fisheye.calibrate`'s output focal length is only
   exactly equivalent to this module's `fx` (pixels-per-radian-of-zenith-angle)
   near the image center for a lens with negligible equisolid distortion —
   if the checkerboard calibration residuals show meaningful equidistant-model
   error near the image edge, `fx` should instead be locally fit against
   several known az/el reference points (e.g. tracked aircraft at varying
   elevation) rather than taken directly from the calibration output.
3. Determine `northOffsetDeg` and `clockwise` empirically (see note above).
4. Re-run `test_skywatch_geometry.js`-style round-trip checks using the
   real calibration numbers as a sanity check before going further.

## Phase 3a — trail detection, proved on real photos (partial)

`examples/skywatch_contrail_detect.js` runs the detection pipeline
standalone on any photo (no camera/ADS-B needed) and was validated against
real phone photos in `/mnt/data/Bilder/DCIM/Draussen/Chemtrails/`.

**What works:** on a clear-sky frame (no natural cirrus), the pipeline
correctly finds and measures real contrails, including a frame with 5
separate trails of varying width/freshness in one shot, while correctly
ignoring the sun's glare, buildings, trees, and power lines. Method:
1. A fixed HSV "whiteness" threshold cannot separate contrail from sky —
   the sky itself has a strong brightness/saturation gradient (Rayleigh
   scattering, paler near the horizon). A **white top-hat filter** (on a
   V−S "whiteness" channel) removes that slow gradient and keeps only
   locally bright streaks.
2. The top-hat also flags building/tree silhouette edges. Fixed by
   rejecting any candidate that sits directly against a "solid object"
   mask (dark, or saturated non-blue hue) — a real trail floats free in
   open sky, an edge artifact hugs the object it came from.

**What doesn't work yet: skies with natural cirrus.** On a frame with
fibrous/banded cirrus cloud, the above method both misses real contrails
and false-positives on cirrus texture, because bounding-box aspect ratio
is a poor proxy for "is this actually a straight man-made trail vs. a
curved natural cloud fiber." The natural fix - detect straightness
directly - was tried and **failed for a specific, informative reason**:
- `cv.LineSegmentDetector` (LSD) on the top-hat image (with terrain
  erased to remove competing edges) found only short fragments of the
  visible diagonal contrail - its local-gradient threshold fragments a
  faint contrail edge exactly the same way it fragments cirrus texture,
  because at this contrast level the two aren't locally distinguishable.
- `cv.HoughLines` (global accumulator, robust to broken/weak edges in
  principle) was tried next specifically because it aggregates evidence
  along an entire candidate line rather than deciding locally. It failed
  for a different, more fundamental reason: **natural fibrous cirrus can
  itself be extremely long and straight** (fibers align with wind shear),
  so the ambient cirrus banding out-voted the real contrail at every
  threshold tested (swept 40 to 1000) - the one true diagonal line never
  appeared as a distinct peak; it was buried under thousands of
  higher-vote near-horizontal lines from the cirrus itself.

**Conclusion - this is a real limitation, not a tuning gap.** Straightness
alone cannot reliably separate contrail from natural cirrus in a single
frame when the cirrus is itself linear. This directly validates why this
project's design anchors detection to ADS-B (phase 4) rather than relying
on vision alone: a trail whose position/orientation/timing matches a real
transponder track is confirmed regardless of how ambiguous it looks
against natural cloud texture. On a cirrus-heavy sky, the pure-vision
detector should be treated as a *candidate generator* for phase 4's
matching step, not a standalone verdict.

## Phase 3 — data feeds

- **ADS-B**: poll readsb's `aircraft.json` on an interval (~1s, matching
  readsb's own update rate). Each aircraft entry needs `lat`, `lon`,
  `alt_baro` (barometric altitude — fine for this use case, no need for
  geometric altitude), `hex` (ICAO24), `flight` (callsign), `gs`
  (groundspeed), `track` (heading).
- **Camera**: continuous frame capture (`js_raspi_cam.cpp`/LCCV), tagged
  with capture timestamp for matching against ADS-B poll timestamps.
- Frame timestamp and ADS-B timestamp need to be reconciled — ADS-B
  position reports lag real aircraft position by roughly the broadcast
  interval (~0.5–1s for airborne position messages), which at cruise
  speed (~250 m/s) is a non-trivial ground distance; either interpolate
  aircraft position at the exact frame timestamp using two consecutive
  ADS-B reports, or accept the resulting pixel-space tolerance window in
  phase 4's matching step instead of correcting for it.

## Phase 4 — matching

For each frame:
1. For every aircraft currently in `aircraft.json` reasonably near
   overhead (e.g. elevation > some cutoff — full 180° fisheye FOV means
   even low elevations are technically visible, but a cutoff avoids noise
   from planes near the horizon where az/el error is largest), compute
   predicted pixel via `azElToPixel`.
2. Run trail/line detection on the frame — reuse one of the existing
   line-based vectorizer methods (`js/vectorizer/vector/methods/lsd.js`,
   `hough-lines.js`, or `fast-line.js`) rather than writing new detection
   from scratch; sky-masking (color/brightness threshold to exclude
   ground/horizon clutter) sits in front of whichever is chosen.
3. Match: is there a detected line segment within tolerance of the
   predicted pixel? Tolerance should account for calibration residual
   error + the ADS-B timing lag from phase 3.
4. Log every frame's outcome — matched (with aircraft metadata) and
   unmatched (both: trails with no aircraft nearby, and aircraft with no
   detected trail — e.g. contrail didn't form, or hasn't yet).

Log format: not yet decided — likely one JSON line per event
(timestamp, frame ref, aircraft hex/flight/alt/speed if matched, matched
pixel, predicted pixel, distance).

## Phase 5 — width-over-time tracking

For a matched trail, track its perpendicular width across subsequent
frames (e.g. sample cross-sections along the line, measure width via
distance-transform or edge-to-edge distance at each sample point).
Optionally correlate width growth rate against the aircraft's altitude,
speed, and (if available) ambient humidity/temperature at that altitude —
this is the actual physical driver of how fast contrail ice crystals
spread (Schmidt-Appleman-adjacent atmospheric conditions), so it's the
natural next signal to bring in if the width data alone isn't enough to
say anything useful.

## Phase 6 — ML detector, trained via ADS-B weak supervision

Goal: replace/augment the classical top-hat detector with a small
semantic-segmentation model, trained without any manual labeling by using
phase 4's ADS-B pixel matches as automatically-generated ground truth.

### Architecture: segmentation CNN, not an RNN

The per-frame question ("which pixels are contrail") is spatial, not
sequential - that's what CNN-based semantic segmentation (U-Net and
descendants) is for. An RNN belongs, if anywhere, on top of the *sequence*
of per-frame detections (tracking width growth smoothly frame-to-frame),
not as the core detector - and even there, a Kalman filter is more
appropriate than a real RNN given how little data this project will ever
have for that specific sub-problem. Concretely:

- **Model**: small U-Net, 4 encoder/decoder levels, narrow channel counts
  (e.g. 16→32→64→128), single input channel (the same V−S "whiteness"
  channel `examples/skywatch_contrail_detect.js` already computes - giving
  the network a head start over raw RGB, since we already know that's the
  channel where the signal lives), single-channel sigmoid output
  (per-pixel contrail probability). Deliberately small: Pi 4 inference,
  and realistically only hundreds-to-low-thousands of training examples
  for a long while.
- **Training**: off-device (a machine with a GPU, or even CPU - the model
  is small), in PyTorch. Not feasible in qjs/C++.
- **Deployment**: export to ONNX, run via `cv.dnn.readNetFromONNX` +
  `blobFromImage` - exactly the pattern already used in this repo
  (`examples/edge_detection_dexined.js`, `examples/neural-font-sr-
  ESPSCx2.js`, `js/cvVectorization.js`'s DNN stage). No new binding work
  needed - `js_dnn.cpp` already covers this.

### Weak-label pipeline: the actual point of the ADS-B anchor

Once phase 4 is live, every frame where a detected candidate's pixel
position matches a real ADS-B-predicted aircraft position is a free,
auto-generated training label - no human ever draws a mask. Before phase
4 exists, the same export pipeline can bootstrap on the classical
detector's output alone (lower-confidence, `source: "classical"` instead
of `"adsb"` - see schema below), so dataset collection can start
immediately instead of waiting on hardware.

**Dataset schema** (one manifest JSON line + one PNG mask per example):
```json
{"id": "20260908-114233-f00123", "image": "crops/20260908-114233-f00123.png",
 "mask": "masks/20260908-114233-f00123.png", "source": "adsb",
 "aircraft": {"hex": "4b1806", "flight": "SWR123", "altM": 10668},
 "capturedAt": "2026-09-08T11:42:33Z"}
```
`source` is either `"adsb"` (phase-4-confirmed - high confidence) or
`"classical"` (top-hat detector output only, used as-is before ADS-B is
running - lower confidence, worth weighting down or excluding once enough
`"adsb"` examples exist). `aircraft` is present only for `"adsb"` rows.

### What's built now (no training data needed for any of this)

- `js/skywatch/export-training-example.js` - given a frame + the
  classical detector's candidate mask (+ optional ADS-B match info),
  writes one manifest row + crop + mask PNG in the schema above. This is
  the piece that turns *any* run of the existing detector into dataset
  collection, starting today, with `source: "classical"` until phase 4
  exists to upgrade it.
- `js/skywatch/train_contrail_unet.py` - the training script. Can't run
  yet (no dataset), but the model definition, data loading (reads the
  manifest schema above), training loop, and ONNX export are all
  independent of whether the dataset has 10 rows or 10,000.
- `examples/skywatch_contrail_infer_dnn.js` - inference stub matching
  `examples/edge_detection_dexined.js`'s pattern: load a `.onnx` model,
  run it on a frame, threshold the probability mask. Expects
  `js/skywatch/models/contrail_unet.onnx` to exist - it won't until the
  training script has been run for real, so this is scaffolding to drop
  the trained model into, not something to run today.

### Not done / explicitly deferred

- Actually training anything - needs real accumulated data, which needs
  phase 3/4 (readsb polling, ADS-B matching) running for real, which
  needs the Pi build. No amount of session time changes that; don't
  re-attempt training without a real dataset.
- Frangi/vesselness classical filter (mentioned in `TODO.md`'s 2026-09-07
  entry as a possible better near-term classical detector) - still worth
  trying before or alongside the ML path, since it needs zero training
  data; not built this session, still open.

## Open questions / decisions still needed

- Exact readsb `aircraft.json` URL/path on this Pi (default is usually
  `http://localhost/tar1090/data/aircraft.json` or similar — confirm once
  readsb is running).
- Elevation cutoff for "worth matching" aircraft.
- Sky-masking approach (fixed color/brightness threshold vs. adaptive).
- Log storage — flat JSON lines file vs. something queryable, given this
  is meant to run continuously and accumulate data over time.

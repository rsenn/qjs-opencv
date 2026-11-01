# Tuning `opencv-imgproc segment` (the magic wand)

Living document: every strategy that could make the tap-to-segment pipeline
better, how to measure it, and a log of what was actually tried. Code under
discussion: `tools/opencv-imgproc.js` (`cmdSegment`, `renderSegment`,
`samDecode*`, `samSoft`, `refineGuided`, `outline`, `findSubjects`) and the
callers in wild-beauty (`server/api/segment.js`, `client/editor/wand.js`,
`wandDown` / `runWand` in `client/components/NodeEditor.js`).

Pipeline per tap set:

```
photo -> SAM image encoder (cached, per photo) -> embeddings
taps  -> SAM mask decoder -> 3 candidate masks, 256x256 logits + predicted IoU
logits -> samSoft (resize, sigmoid) -> refineGuided / refineMatting
       -> findSubjects (components) -> outline (contour, approxPolyDP)
```

## 1. How to measure

No hand-made ground truth is needed. Use public matting sets with real alpha
mattes, plus reference-model and A/B methods for photos of your own.

| Source | Use | Notes |
|---|---|---|
| AIM-500 | main benchmark: 500 natural photos, 424 SO / 43 STM / 33 NS; categories animal 200, portrait 100, plant 75, furniture 45, toy 36, transparent 34, fruit 10 | MIT release agreement, read it first; mirror `nobg/AIM-500` on Hugging Face (Google Drive hits its quota) |
| P3M-10k / P3M-500-NP | portraits and hair only | several GB, Google Drive; only if hair edges become the focus |
| big-model reference | label own photos: `--model sam` plus box and several taps as pseudo truth | consistent, not exact; good for comparing settings |
| A/B page | edge taste on own photos | win rate over about 50 photos |
| no-truth metrics | regression guard | tap-jitter stability, mean gradient along the contour, share of picks over 60% coverage |

Interactive protocol (as in RITM / SAM papers): tap 1 at the interior point
farthest from the boundary of the ground-truth object; every next tap at the
centre of the largest error region (positive when foreground was missed,
negative when it spilled); stop at IoU 0.9 or 10 taps.

Metrics per image: IoU at 1, 2, 3, 5 taps (mIoU@k), NoC@85 and NoC@90 (taps
needed), boundary F-score (3 px tolerance), latency of preview and final,
coverage of the picked candidate. Report overall and per category and per type
(SO, STM, NS).

Ground truth is a soft alpha: binarise at 127 for IoU, keep the soft matte for
SAD / MSE only when `--refine matting` is judged.

Harness: `tools/tuning/` (see section 5 log). Dataset lives outside the repo
in `~/.local/share/opencv-imgproc-eval/aim500/{img,gt}/NNN.{jpg,png}`.

## 2. Tuning strategies

Ranked by expected payoff; "knob" names the code or option that changes.

### 2.1 Candidate selection

- Server hardcodes `--pick 2` (whole object); the client then steps one
  smaller when coverage is over `WAND_MAX_COVER` (0.6). Replace with a data
  driven rule: predicted IoU, coverage, tap depth inside the mask (distance
  transform at the tap), and tap-under-mask for every positive tap.
- Learn the rule offline: from the harness collect all three candidates'
  features and their true IoU, fit a tiny decision tree or logistic model,
  hard-code the thresholds.
- Per-tap-count rule: one tap prefers part/whole by score; several taps use
  only the multi-prompt mask.
- Knob: `--pick`, `WAND_MAX_COVER`, `picked` in `renderSegment`.

### 2.2 Multi-tap prompting

- `samDecodeOrig` sends a zero `mask_input` and `has_mask_input = 0`. SAM is
  trained to take the previous low-res logits as a mask prompt; feed the best
  logits of the last run back (the client resends all taps anyway, so the
  server must cache by photo and tap prefix, or the tool accepts `--mask-in`).
- Candidate index mapping for several taps is `[0, 2, 3]`; verify that
  output 0 is the best one, and try returning only 0.
- Negative taps: check that label 0 reaches the decoder as 0 (not -1 padding).
- Tap placement tolerance: snap a tap to the nearest superpixel centre or
  nudge it away from edges (distance transform of Canny edges).
- Knob: `samDecodeOrig`, `samDecode`, `parsePrompts`.

### 2.3 Logit to matte

- `samSoft` upsamples the 256x256 logits with bicubic, then applies a sigmoid
  with slope 2. Compare: bilinear logit upsample, slope 1 / 2 / 4 / 8, a
  threshold bias, and cropping the padded square before the resize (already
  done) versus after.
- Upsample the logits in two steps (256 to 1024 to photo size) to keep edges
  from drifting.
- Knob: `samSoft`.

### 2.4 Edge refinement

- `refineGuided`: radius `guideRadius` (0.6% of the long side, min 4), `eps`
  1e-3, final `contrast(g, 4)`. Grid search radius 0.3 to 1.2%, eps 1e-4 to
  1e-2, contrast 2 to 8.
- Replace the guide: Lab or edge-preserved gray instead of BGR; add a second
  pass with a small radius.
- `refineMatting` (`alphamat.infoFlow`): trimap `band` is 1.5% of the work
  size, `workSize` 1024. Measure SAD gain against time; keep it for hair and
  transparent categories only (could be chosen from the soft-mask edge
  statistics).
- Fast alternatives: `cv.ximgproc.fastGlobalSmootherFilter`, joint bilateral,
  GrabCut seeded with the SAM mask in a narrow band around the contour.
- Knob: `--refine`, `--work-size`, constants above.

### 2.5 Topology and outline

- `outline()` uses `RETR_EXTERNAL` and the largest contour: holes vanish
  (handles, rings) and the component under the tap may be ignored.
  Use `RETR_CCOMP` / `RETR_TREE`, keep the component under any positive tap,
  and pass holes along (the clip then needs an even-odd fill).
- `approxPolyDP` epsilon: scale with perimeter (0.1 to 0.5% of it) rather
  than a fixed pixel value; compare vertex count against boundary F.
- Remove islands under `minArea` (0.0002) after thresholding; fill holes
  smaller than a few pixels.
- Smooth the polygon (Chaikin or a few morphological open/close steps)
  before simplifying.
- Knob: `outline`, `findSubjects`, `--min-area`.

### 2.6 Model

- `--model mobilesam` (default) versus `sam` (larger encoder): measure IoU
  gain and latency; maybe default to `sam` once a photo's embedding is cached.
- Quantised ONNX or fp16 encoder for speed; check the IoU loss.
- Test-time augmentation: horizontal flip of image and taps, average logits
  (2x decode cost only if the embedding of the flipped photo is cached).
- Encode at a larger input for small objects (tiles) when the object is under
  a few percent of the photo.
- Knob: `MODELS`, `samEmbeddings`.

### 2.7 Latency

- Two phases already exist (`--phases preview,final`, `--view-size 1024`).
  Tune `--view-size` (768 / 1024 / 1536) against preview quality.
- Run `prepare` on upload so the first tap only decodes.
- Skip the other two candidates' refine step when `--candidates 3` is not
  needed (refine is per candidate); compute neighbours lazily on "bigger /
  smaller".
- Abort stale runs (done: the server kills the process on a new tap); keep a
  long-lived worker so ONNX and the embedding stay in memory.

### 2.8 Client heuristics

- `WAND_MAX_COVER` 0.6 and the "one smaller" retry cost a second run; replace
  by the learned rule in 2.1 so the first answer is right.
- Bigger / smaller buttons: order candidates by area rather than SAM index.
- Show the negative-tap effect immediately (preview) and defer the final.

### 2.9 Robustness

- Tiny or huge photos (`--max-pixels`), very low contrast, grayscale, alpha
  inputs: add them to the eval set as a "stress" category.
- Tap jitter stability (shift a tap by 1 to 3% of the photo; IoU between runs
  should stay over 0.95) as a regression test.

## 3. Method of the SAM optimisation

1. Baseline: run the harness on AIM-500 with the tool's current defaults.
2. One change at a time, in the order 2.2, 2.1, 2.3, 2.4, 2.5, 2.6.
3. Keep a change when mIoU@1 or NoC@90 improves by more than the noise (use
   the same simulated taps, paired comparison, bootstrap CI over images)
   without a latency regression above 20%.
4. Parameters that win become defaults in `cmdSegment` / `renderSegment`; the
   wild-beauty server only passes what is not default.
5. Add each experiment to the log below with date, change, numbers.

Beware overfitting: tune on a fixed 250-image half, report on the other half;
AIM-500 skews to animals (40%), so also read the per-category numbers.

## 4. Datasets: how it was fetched

- Official Google Drive folder hit the per-file download quota (76 masks, no
  photos). The Hugging Face mirror `nobg/AIM-500` (parquet, MIT, soft mattes
  kept as given) worked; `source.parquet` is kept next to the extracted files.
- Extraction: `tools/tuning/extract.py` (pyarrow + pillow in a venv, run with
  `python -I`), writes `img/NNN.jpg` (q95) and `gt/NNN.png` (8-bit alpha).

## 5. Experiment log

Newest last. Numbers are from `tools/tuning/results/*.csv`.

- 2026-11-01, AIM-500 downloaded and extracted (500 photos, 248 MB).
- 2026-11-01, harness `tools/tuning/{run,report}.py`, baseline on a fixed
  random 100-image subset (seed 1), up to 4 simulated taps, tool defaults,
  `--candidates 3 --refine guided` (`results/baseline-2026-11-01.csv`):

  | policy | mIoU@1 | @2 | @3 | @4 | NoC@85 | NoC@90 | bf@1 |
  |---|---|---|---|---|---|---|---|
  | editor (cand 2, or 1 over 60% coverage) | 0.840 | 0.881 | 0.875 | 0.878 | 1.68 | 1.86 | 0.566 |
  | SAM predicted score | 0.708 | 0.816 | 0.815 | 0.826 | 1.95 | 2.12 | 0.472 |
  | candidate 0 / 1 / 2 only | 0.464 / 0.647 / 0.822 | | | | | | |
  | oracle (best candidate) | 0.857 | 0.892 | 0.893 | 0.893 | 1.67 | 1.80 | 0.581 |

  mean latency 2.8 s per run (embedding cache warm for most). Findings:
  - the editor rule is within 0.02 mIoU@1 of the oracle: candidate choice (2.1)
    has little left to gain; SAM's own score is a worse picker than the
    coverage rule, so do not switch to it.
  - extra taps barely help and mIoU can drop from tap 2 to 3 (0.881 to 0.875):
    each run starts from zero `mask_input`, so a new tap can undo earlier
    ones. 2.2 (feed the previous logits back) is the next experiment.
  - boundary F is only 0.57 at 3 px while IoU is 0.84: the edges, not the
    object choice, are the weak part, so 2.3 and 2.4 matter.
  - the tool fails outright ("no subject above --min-area") on small
    objects (image 031, 1% of the photo): score 0 in the harness; check
    `--min-area` and the decoder's candidate for tiny masks.
- 2026-11-01, edge constants (`--guide-eps`, `--guide-radius`, `--soft-slope`,
  `--contrast`), 1 tap, first 60 images of the subset, control mIoU 0.834 /
  boundary F 0.544:

  | setting | mIoU@1 | bf@1 |
  |---|---|---|
  | guide-eps 1e-4 / 1e-3 (control) | 0.834 / 0.834 | 0.541 / 0.544 |
  | guide-eps 0.01 | 0.839 | 0.589 |
  | guide-eps 0.03 / 0.1 / 0.3 | 0.838 / 0.837 / 0.836 | 0.582 / 0.572 / 0.560 |
  | guide-radius 0.003 / 0.012 / 0.02 (eps 1e-3) | 0.835 / 0.819 / 0.797 | 0.555 / 0.507 / 0.432 |
  | guide-radius 0.003 with eps 0.03 / 0.1 | 0.837 / 0.837 | 0.569 / 0.564 |
  | soft-slope 1 / 4 / 8 | 0.833 / 0.834 / 0.835 | 0.531 / 0.550 / 0.552 |
  | contrast 2 / 8 | 0.834 / 0.834 | 0.544 / 0.544 |

  `--contrast` cannot change the binary mask: a stretch around 0.5 never moves
  the 0.5 threshold, it only shapes the soft alpha edge (needs a matte metric).
  Held-out check on the other 40 photos (images 60..99 of the shuffle): eps
  1e-3 gives mIoU 0.849 / bf 0.598, eps 0.01 gives 0.854 / 0.641, so 0.01 is
  the `segment` default now (`cutout` keeps 1e-3, not validated there).
- 2026-11-01, `--groups auto` (a positive tap away from the found mask starts
  its own group, the matte is the union), tool with eps 0.01:
  - two-object composites (`tools/tuning/mkpair.py`, 60 images, 3 taps): editor
    policy mIoU@2 0.558 to 0.664, mIoU@3 0.598 to 0.724, NoC@90 3.80 to 3.32;
    tap 1 is unchanged (0.490).
  - single-object photos (60 images, 4 taps): mIoU@2 0.885 to 0.882, @3 0.879
    to 0.863, @4 0.887 to 0.883, NoC@90 1.92 to 1.87, so the only loss is a
    small dip at tap 3; `auto` is the server default, `single` stays an option.
- 2026-11-01, outline tidy for the clip polygons (`--outline-smooth`,
  `--outline-grow`, `--outline-vertices`): one contour 522 vertices
  at 1.5 px, 31 with smooth 0.004 + 40 vertices, 38 with grow 0.01 added.
  Cutout clips ask for smooth 0.004, grow 0.01, 48 vertices (the alpha does the
  fine cutting), the wand preview for smooth 0.003, 80 vertices.
- 2026-11-01, rest-image fill (the hole a taken-out subject leaves), same 1024
  x 768 photo, robot hole, 8 cores, onnxruntime unless noted:

  | model | file | load | per fill |
  |---|---|---|---|
  | Telea (`cv.inpaint`) | none | none | 0.2 s |
  | MI-GAN pipeline v2 (MIT) | 28 MB | 0.3-0.4 s | 1.0-1.7 s |
  | LaMa fp32 (Carve) | 208 MB | 11 s | 2.7-4.1 s |
  | LaMa quantized (OpenCV zoo) | 88 MB | 10 s | 4.9 s |
  | LaMa quantized, OpenCV dnn (`--rest-fill lama`) | 88 MB | 30 s | 7 s |

  Same hole, MI-GAN and LaMa fp32 look alike; the over-grown SAM mask (robot
  plus glass plus shoulder) smears either, so the mask is the weak part, not
  the fill. OpenCV dnn loads MI-GAN in 0.2 s but returns a wrong output size
  (589824 values for a 1024x768 pipeline, 2359296 expected): not used. The
  server runs `tools/inpaint.py` (python onnxruntime, MI-GAN pipeline), 2.8 s
  wall from a cold process.
- 2026-11-01, `--region JSON` (a manual clip as polygons in 0..1, the matte is
  cut to it): tap on the robot with the region on the left half gives
  coverage 0.127, without it SAM's "whole" took the glass too (0.207).
- 2026-11-01, candidate pick rule search (`results/baseline-2026-11-01.csv`,
  tap 1, 100 photos): rules of the form "candidate 1 when the whole is over c
  of the photo, or when its SAM score is under s and candidate 1 scores d
  higher" fitted on one half reach 0.835 / 0.829 on their own half but 0.811 /
  0.726 on the other, against the editor's coverage rule at 0.827 / 0.853: no
  rule beats `WAND_MAX_COVER` 0.6 out of sample, so candidate choice stays.
  In scenes with several objects "whole" can still swallow a neighbour (the
  glass next to the robot): that is what the "smaller" chip, an exclude tap
  (it joins the group it falls in) and a manual clip first (`--region`) are for.
- 2026-11-01, end to end through `segmentRoute` (scratch dirs, take out on, one
  tap on the robot of a 1024x768 composite, CPU shared with a measurement
  run): preview 4.3 s (cold embedding), final 5.7 s, cut announced at 5.75 s,
  filled photo (`inpaint.py`) at 9.7 s.
- 2026-11-01, `--mask-prompt on` (each decode gets the previous logits as SAM's
  mask input; groups join and single multi-tap alike), same sets as above:

  | set | policy | mIoU@2 | @3 | @4 | NoC@90 |
  |---|---|---|---|---|---|
  | two-object composites (3 taps) | groups auto | 0.664 | 0.724 | | 3.32 |
  | | groups auto + mask prompt | 0.697 | 0.787 | | 3.20 |
  | single-object, 60 photos | groups single | 0.885 | 0.879 | 0.887 | 1.92 |
  | | groups single + mask prompt | 0.853 | 0.879 | 0.875 | 1.90 |
  | | groups auto | 0.882 | 0.863 | 0.883 | 1.87 |
  | | groups auto + mask prompt | 0.850 | 0.871 | 0.869 | 1.90 |

  It helps scenes with several objects (+0.06 at tap 3) and costs about 0.03
  at tap 2 on single objects, the common case, so it stays off by default.
  Idea not tried: use it only for a group that already has more than one tap,
  or only when a tap would start a new group.
- 2026-11-01, select / apply split for the wand (wild-beauty `WAND.md`):
  `--masks-only --view-size 1024 --candidates 3` returns every candidate as
  outlines and writes no file; the apply step renders only the chosen
  candidate at full size, with edge cleanup and colour estimation cropped to
  the area around the alpha (output bit-identical). One tap on a 4032x3024
  upload: select 2.3 s cold, 0.74 s warm; apply 5.4 s (was 15 s with all three
  candidates rendered); `inpaint.py` fill 3.8 s, `--no-fill` 2.5 s (PNG
  encoding of 12 MP).
- 2026-11-01, `--exclude object` (a negative tap is also decoded alone as an
  object, its largest candidate without a positive tap, and that matte is taken
  out of every candidate; the tap stays a negative point too), groups auto,
  same images as the baselines (`pairs-auto-ex2`, `reg-auto-ex2`):

  | set | exclude | mIoU@2 | @3 | @4 | NoC@90 |
  |---|---|---|---|---|---|
  | two-object composites (3 taps) | point | 0.664 | 0.724 | | 3.32 |
  | | object | 0.709 | 0.814 | | 3.22 |
  | single-object, 60 photos | point | 0.882 | 0.863 | 0.883 | 1.87 |
  | | object | 0.862 | 0.857 | 0.881 | 1.87 |

  Adopted for the editor (exclude is mostly used against a neighbour). A first
  version without the negative point lost the hint: SAM's "whole" grew from a
  protea to its whole bush. Note: an earlier run of this was spoiled by two
  harness runs writing the same CSV; check `pgrep -af run.py` before starting
  one (and `pkill` patterns: `\|` is a literal bar in pkill's ERE).
- 2026-11-01, apply speed (12 MP, subject 7%): the guided filter is cropped to
  where the matte is above 0.05 plus 3 radii (the contrast steepening zeroes
  everything below 0.375), and `encode` zeroes the colour under alpha 0 (the
  PNG was 20 MB, the whole photo hidden in it; now 2.2 MB, level 1 deflate
  1.24 s to 0.53 s). Total 4.27 s to 2.48 s, alpha and visible colour
  bit-identical.
- 2026-11-01, latency pass (wild-beauty `WAND.md` B8): `loadNet` keeps a net
  per run; the SAM decoder (16 MB, 125 to 175 ms to load) was loaded again for
  every decode, and groups plus exclude taps decode several times: a 4-tap
  select 2.37 s to 1.34 s, outlines identical. One decoder forward in OpenCV's
  dnn is 65 to 75 ms; `orig_im_size` 256 instead of 1024 saves 8 ms and
  changes the coordinates' frame, not done. OpenCV's WebP encoder needs 0.69 s
  for a 2048 px RGBA, so writing display copies in the tool was dropped.

import {
  Mat,
  Point,
  Size,
  Rect,
  CV_8UC1,
  CV_8UC3,
  COLOR_BGR2GRAY,
  COLOR_GRAY2BGR,
  LINE_AA,
  LINE_8,
  FILLED,
  cvtColor,
  imread,
  imshow,
  waitKey,
  skeletonization,
  pixelNeighborhood,
  pixelFindValue,
  traceSkeleton,
  drawLine,
  drawCircle,
} from 'opencv';

function hsv2bgr(h, s, v) {
  const c = v * s;
  const hp = (h % 360) / 60;
  const x = c * (1 - Math.abs((hp % 2) - 1));
  let r = 0, g = 0, b = 0;
  if(hp < 1)      [r, g, b] = [c, x, 0];
  else if(hp < 2) [r, g, b] = [x, c, 0];
  else if(hp < 3) [r, g, b] = [0, c, x];
  else if(hp < 4) [r, g, b] = [0, x, c];
  else if(hp < 5) [r, g, b] = [x, 0, c];
  else            [r, g, b] = [c, 0, x];
  const m = v - c;
  return [Math.round((b + m) * 255), Math.round((g + m) * 255), Math.round((r + m) * 255), 255];
}

function main(filename = 'tests/test_linesegmentdetector.jpg') {
  console.log('input:', filename);

  const input = imread(filename);
  if(input.empty) {
    console.log('failed to load', filename);
    return 1;
  }
  console.log('size:', input.size, 'channels:', input.channels);

  // Stage 1: thinning. skeletonization() handles BGR->gray + Otsu internally.
  const skel = new Mat();
  skeletonization(input, skel);
  console.log('skeleton: type=0x' + skel.type.toString(16), 'size=', skel.size);

  // Stage 2: 8-neighbour degree map. Source must be a cv.Mat (not UMat).
  const neighborhood = new Mat();
  pixelNeighborhood(skel, neighborhood);

  // Convenience seeds: endpoints (degree=1) and junctions (degree>=3).
  const endpoints = pixelFindValue(neighborhood, 1);
  const junctions = [
    ...pixelFindValue(neighborhood, 3),
    ...pixelFindValue(neighborhood, 4),
    ...pixelFindValue(neighborhood, 5),
  ];
  console.log('endpoints:', endpoints.length, 'junctions:', junctions.length);

  // Stage 3: trace polylines. The 4-arg form also fills the internal
  // neighbourhood and mapping mats for inspection.
  const contours = [];
  const dbgNeighborhood = new Mat();
  const dbgMapping = new Mat();
  const count = traceSkeleton(skel, contours, dbgNeighborhood, dbgMapping);

  const totalPoints = contours.reduce((n, c) => n + c.length, 0);
  console.log('contours:', count, 'total points:', totalPoints);
  if(count > 0) {
    const lengths = contours.map(c => c.length).sort((a, b) => b - a);
    console.log('contour length: max=', lengths[0], 'min=', lengths[lengths.length - 1], 'median=', lengths[lengths.length >> 1]);
  }

  // --- Visualization ---

  // Side-by-side canvas: [original | skeleton | traced polylines].
  const { width, height } = input.size;
  const canvas = new Mat(height, width * 3, CV_8UC3);

  // Panel 0: original image (force 3-channel if grayscale).
  let bgrInput;
  if(input.channels === 1) {
    bgrInput = new Mat();
    cvtColor(input, bgrInput, COLOR_GRAY2BGR);
  } else {
    bgrInput = input;
  }
  bgrInput.copyTo(canvas(new Rect(0, 0, width, height)));

  // Panel 1: skeleton as a BGR image.
  const skelBgr = new Mat();
  cvtColor(skel, skelBgr, COLOR_GRAY2BGR);
  skelBgr.copyTo(canvas(new Rect(width, 0, width, height)));

  // Panel 2: traced polylines drawn on a dark background, one color per contour.
  const traced = new Mat(height, width, CV_8UC3);
  traced.setTo([20, 20, 20, 255]);

  for(let i = 0; i < contours.length; i++) {
    const c = contours[i];
    if(c.length < 2) {
      // Single-pixel contour: mark it so it isn't lost.
      drawCircle(traced, new Point(c[0].x, c[0].y), 1, hsv2bgr((i * 37) % 360, 0.9, 1.0), FILLED, LINE_8);
      continue;
    }
    const color = hsv2bgr((i * 37) % 360, 0.9, 1.0);
    for(let j = 1; j < c.length; j++) {
      drawLine(traced, new Point(c[j - 1].x, c[j - 1].y), new Point(c[j].x, c[j].y), color, 1, LINE_AA);
    }
  }

  // Overlay endpoints (green) and junctions (red).
  for(const p of endpoints)
    drawCircle(traced, new Point(p.x, p.y), 2, [0, 255, 0, 255], 1, LINE_AA);
  for(const p of junctions)
    drawCircle(traced, new Point(p.x, p.y), 2, [0, 0, 255, 255], 1, LINE_AA);

  traced.copyTo(canvas(new Rect(width * 2, 0, width, height)));

  imshow('input | skeleton | traced', canvas);
  console.log('press any key to exit (q/Esc to quit)');

  while(true) {
    const key = waitKey(0);
    if(key === -1) continue;
    if(key === 'q' || key === 113 || key === '\x1b' || key === 27) break;
    if(typeof key === 'number' && key >= 0) break;
  }

  return 0;
}

main(...scriptArgs.slice(1));

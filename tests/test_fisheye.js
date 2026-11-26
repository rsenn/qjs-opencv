import * as cv from 'opencv';

/* Fisheye calibration and undistortion on synthetic data: project a known
 * chessboard through a known K, D (cv.fisheye.projectPoints), calibrate from
 * those views (cv.fisheye.calibrate) and check that K and D come back, then
 * undistort (maps + remap, undistortImage, undistortPoints) and check that
 * the image of a straight 3D line is straight again. Also covers the
 * pinhole calib3d functions that the fisheye workflow needs. */

let failed = 0;

function ok(name, fn) {
  try {
    fn();
    console.log('OK  ', name);
  } catch(e) {
    failed++;
    console.log('FAIL', name, '-', (e && e.message) || e);
  }
}

function assert(cond, msg) {
  if(!cond) throw new Error(msg || 'assertion failed');
}

function near(a, b, tol, what) {
  if(!(Math.abs(a - b) <= tol)) throw new Error(`${what}: ${a} vs ${b} (tol ${tol})`);
}

const W = 640, H = 480;
const K_TRUE = [420, 0, 320, 0, 420, 240, 0, 0, 1];
const D_TRUE = [0.08, -0.03, 0.01, -0.002];
const K = cv.matFromArray(3, 3, cv.CV_64F, K_TRUE);
const D = cv.matFromArray(4, 1, cv.CV_64F, D_TRUE);

/* deterministic pseudo random numbers so the test is repeatable */
let seed = 12345;
const rnd = () => ((seed = (seed * 1664525 + 1013904223) >>> 0) / 4294967296);
const gauss = () => Math.sqrt(-2 * Math.log(rnd() + 1e-12)) * Math.cos(2 * Math.PI * rnd());

/* 9x6 inner corners, 3 cm squares, centred on the origin */
const OBJ = [];
for(let y = 0; y < 6; y++) for(let x = 0; x < 9; x++) OBJ.push((x - 4) * 0.03, (y - 2.5) * 0.03, 0);
const objMat = cv.matFromArray(54, 1, cv.CV_64FC3, OBJ);

function rotvec(i) {
  return [(rnd() - 0.5) * 0.9, (rnd() - 0.5) * 0.9, (rnd() - 0.5) * 0.6];
}

function project(obj, rv, tv, Kmat = K, Dmat = D) {
  const img = new cv.Mat();
  cv.fisheye.projectPoints(obj, img, cv.matFromArray(3, 1, cv.CV_64F, rv), cv.matFromArray(3, 1, cv.CV_64F, tv), Kmat, Dmat);
  return img;
}

const inside = img => {
  for(let i = 0; i < img.rows * img.cols; i++) {
    const x = img.data64F[2 * i], y = img.data64F[2 * i + 1];
    if(!(x > 8 && x < W - 8 && y > 8 && y < H - 8)) return false;
  }
  return true;
};

/* views: random pose, accept only if every corner is inside the image */
const objectPoints = [], imagePoints = [];
while(objectPoints.length < 16) {
  const rv = rotvec(), tv = [(rnd() - 0.5) * 0.12, (rnd() - 0.5) * 0.08, 0.17 + rnd() * 0.12];
  const img = project(objMat, rv, tv);

  if(!inside(img)) continue;

  /* 0.1 px measurement noise */
  for(let i = 0; i < img.data64F.length; i++) img.data64F[i] += gauss() * 0.1;
  objectPoints.push(objMat);
  imagePoints.push(img);
}

const Kc = new cv.Mat(), Dc = new cv.Mat();
let rms;

ok('fisheye.projectPoints (rvec/tvec form) projects inside the image', () => {
  const img = project(objMat, [0, 0, 0], [0, 0, 0.2]);

  assert(img.rows * img.cols == 54, 'point count');
  assert(img.data64F[0] > 50 && img.data64F[0] < 140, 'x of the first corner ' + img.data64F[0]);
});

ok('fisheye.calibrate recovers K and D', () => {
  const rvecs = [], tvecs = [];

  rms = cv.fisheye.calibrate(objectPoints, imagePoints, [W, H], Kc, Dc, rvecs, tvecs, cv.fisheye.CALIB_RECOMPUTE_EXTRINSIC | cv.fisheye.CALIB_FIX_SKEW);

  assert(typeof rms == 'number', 'returns the RMS error, got ' + rms);
  assert(rms < 0.5, 'rms ' + rms);
  assert(rvecs.length == 16 && tvecs.length == 16, 'rvecs/tvecs filled: ' + rvecs.length);

  const k = Kc.data64F;
  near(k[0], 420, 4, 'fx');
  near(k[4], 420, 4, 'fy');
  near(k[2], 320, 3, 'cx');
  near(k[5], 240, 3, 'cy');

  const d = Dc.data64F;
  near(d[0], D_TRUE[0], 0.03, 'k1');
  near(d[1], D_TRUE[1], 0.05, 'k2');
  console.log('     rms', rms.toFixed(4), 'K', Array.from(k).map(v => +v.toFixed(2)).join(','), 'D', Array.from(d).map(v => +v.toFixed(4)).join(','));
});

/* a straight 3D line (z = 0.25, y = 0.1, x from -0.2 to 0.2) drawn as dense dots in the fisheye image */
function lineImage() {
  const n = 1500, pts = [];
  for(let i = 0; i < n; i++) pts.push(-0.2 + (0.4 * i) / (n - 1), 0.1, 0.25);
  const obj = cv.matFromArray(n, 1, cv.CV_64FC3, pts);
  const proj = project(obj, [0, 0, 0], [0, 0, 0]);
  const img = new cv.Mat(H, W, cv.CV_8UC1);

  img.setTo(0);
  for(let i = 0; i < n; i++) {
    const x = Math.round(proj.data64F[2 * i]), y = Math.round(proj.data64F[2 * i + 1]);
    for(let dy = -1; dy <= 1; dy++)
      for(let dx = -1; dx <= 1; dx++)
        if(x + dx >= 0 && x + dx < W && y + dy >= 0 && y + dy < H) img.data[(y + dy) * W + x + dx] = 255;
  }
  return { img, proj };
}

/* RMS distance of the white pixels from their total-least-squares line */
function straightness(img) {
  const px = [];
  const w = img.cols, h = img.rows;

  for(let y = 0; y < h; y++) for(let x = 0; x < w; x++) if(img.data[y * w + x] > 128) px.push([x, y]);
  assert(px.length > 200, 'only ' + px.length + ' line pixels');

  let mx = 0, my = 0;
  for(const [x, y] of px) { mx += x; my += y; }
  mx /= px.length; my /= px.length;

  let sxx = 0, sxy = 0, syy = 0;
  for(const [x, y] of px) { sxx += (x - mx) ** 2; sxy += (x - mx) * (y - my); syy += (y - my) ** 2; }

  /* smallest eigenvalue of the covariance = squared RMS distance to the line */
  const tr = sxx + syy, det = sxx * syy - sxy * sxy;
  const lmin = tr / 2 - Math.sqrt(Math.max(0, (tr * tr) / 4 - det));
  return Math.sqrt(lmin / px.length);
}

ok('the fisheye image of a straight line is curved', () => {
  const { img } = lineImage();
  const s = straightness(img);

  assert(s > 2, 'distorted line straightness rms ' + s);
  console.log('     distorted line rms deviation', s.toFixed(2), 'px');
});

ok('initUndistortRectifyMap + remap straighten the line', () => {
  const { img } = lineImage();
  const R = cv.matFromArray(3, 3, cv.CV_64F, [1, 0, 0, 0, 1, 0, 0, 0, 1]);
  const Knew = new cv.Mat();

  cv.fisheye.estimateNewCameraMatrixForUndistortRectify(Kc, Dc, [W, H], R, Knew, 0.0);

  const map1 = new cv.Mat(), map2 = new cv.Mat();
  cv.fisheye.initUndistortRectifyMap(Kc, Dc, R, Knew, [W, H], cv.CV_16SC2, map1, map2);

  const out = new cv.Mat();
  cv.remap(img, out, map1, map2, cv.INTER_LINEAR, cv.BORDER_CONSTANT, new cv.Scalar(0, 0, 0, 0));

  /* the drawn line is 3 px thick: a perfectly straight one measures about 0.8 */
  const s = straightness(out);
  assert(s < 1.5, 'undistorted line straightness rms ' + s);
  console.log('     remapped line rms deviation', s.toFixed(2), 'px');
});

ok('fisheye.undistortImage straightens the line', () => {
  const { img } = lineImage();
  const out = new cv.Mat();

  cv.fisheye.undistortImage(img, out, Kc, Dc, Kc);

  const s = straightness(out);
  assert(s < 1.5, 'undistortImage straightness rms ' + s);
});

ok('fisheye.undistortPoints gives collinear points', () => {
  const { proj } = lineImage();
  const R = cv.matFromArray(3, 3, cv.CV_64F, [1, 0, 0, 0, 1, 0, 0, 0, 1]);
  const out = new cv.Mat();

  cv.fisheye.undistortPoints(proj, out, Kc, Dc, R, Kc);

  /* line through the first and last point, max distance of the others */
  const n = out.rows * out.cols, p = out.data64F;
  const x0 = p[0], y0 = p[1], x1 = p[2 * n - 2], y1 = p[2 * n - 1];
  const len = Math.hypot(x1 - x0, y1 - y0);
  let maxd = 0;

  for(let i = 0; i < n; i++) maxd = Math.max(maxd, Math.abs((x1 - x0) * (y0 - p[2 * i + 1]) - (x0 - p[2 * i]) * (y1 - y0)) / len);
  assert(maxd < 0.5, 'max deviation from the line ' + maxd);
});

ok('calibrated K, D match a repeated projection (reprojection error)', () => {
  const rv = [0.1, -0.2, 0.05], tv = [0.01, -0.02, 0.22];
  const a = project(objMat, rv, tv);
  const b = project(objMat, rv, tv, Kc, Dc);
  let maxe = 0;

  for(let i = 0; i < a.data64F.length; i += 2) maxe = Math.max(maxe, Math.hypot(a.data64F[i] - b.data64F[i], a.data64F[i + 1] - b.data64F[i + 1]));
  assert(maxe < 1.5, 'max reprojection error ' + maxe);
});

/* --- the pinhole calib3d functions used by the same workflow --- */

const KP = cv.matFromArray(3, 3, cv.CV_64F, [500, 0, 320, 0, 500, 240, 0, 0, 1]);
const DP = cv.matFromArray(5, 1, cv.CV_64F, [-0.2, 0.05, 0.001, -0.001, 0]);

ok('general constants are exported', () => {
  for(const n of ['CALIB_USE_INTRINSIC_GUESS', 'CALIB_FIX_PRINCIPAL_POINT', 'CALIB_ZERO_TANGENT_DIST', 'CALIB_RATIONAL_MODEL', 'CALIB_FIX_K1', 'CALIB_CB_SYMMETRIC_GRID', 'SOLVEPNP_ITERATIVE', 'SOLVEPNP_IPPE', 'RANSAC', 'LMEDS'])
    assert(typeof cv[n] == 'number', n + ' missing');
});

ok('projectPoints + solvePnP recovers the pose', () => {
  const rv = cv.matFromArray(3, 1, cv.CV_64F, [0.2, -0.1, 0.3]), tv = cv.matFromArray(3, 1, cv.CV_64F, [0.05, -0.03, 0.6]);
  const img = new cv.Mat();

  cv.projectPoints(objMat, rv, tv, KP, DP, img);
  assert(img.rows * img.cols == 54, 'projected count');

  const rv2 = new cv.Mat(), tv2 = new cv.Mat();
  const found = cv.solvePnP(objMat, img, KP, DP, rv2, tv2, false, cv.SOLVEPNP_ITERATIVE);

  assert(found === true, 'solvePnP');
  for(let i = 0; i < 3; i++) {
    near(rv2.data64F[i], rv.data64F[i], 1e-3, 'rvec' + i);
    near(tv2.data64F[i], tv.data64F[i], 1e-3, 'tvec' + i);
  }

  const rv3 = new cv.Mat(), tv3 = new cv.Mat(), inl = new cv.Mat();
  assert(cv.solvePnPRansac(objMat, img, KP, DP, rv3, tv3, false, 100, 4.0, 0.99, inl, cv.SOLVEPNP_ITERATIVE), 'solvePnPRansac');
  near(tv3.data64F[2], 0.6, 1e-2, 'ransac tvec z');
});

ok('Rodrigues round trip', () => {
  const rv = cv.matFromArray(3, 1, cv.CV_64F, [0.3, -0.2, 0.5]);
  const Rm = new cv.Mat(), back = new cv.Mat();

  cv.Rodrigues(rv, Rm);
  assert(Rm.rows == 3 && Rm.cols == 3, 'R is 3x3');
  cv.Rodrigues(Rm, back);
  for(let i = 0; i < 3; i++) near(back.data64F[i], rv.data64F[i], 1e-9, 'rvec' + i);
});

ok('undistort / initUndistortRectifyMap / getOptimalNewCameraMatrix', () => {
  const img = new cv.Mat(H, W, cv.CV_8UC1);
  img.setTo(128);

  const roi = { x: 0, y: 0, width: 0, height: 0 };
  const Knew = cv.getOptimalNewCameraMatrix(KP, DP, [W, H], 0.5, [W, H], roi, false);
  assert(Knew.rows == 3 && Knew.cols == 3, 'Knew 3x3');
  assert(roi.width > 0 && roi.height > 0, 'validPixROI filled ' + JSON.stringify(roi));

  const out = new cv.Mat();
  cv.undistort(img, out, KP, DP, Knew);
  assert(out.rows == H && out.cols == W, 'undistort size');

  const m1 = new cv.Mat(), m2 = new cv.Mat();
  cv.initUndistortRectifyMap(KP, DP, new cv.Mat(), Knew, [W, H], cv.CV_32FC1, m1, m2);
  assert(m1.rows == H && m1.cols == W && m1.type() == cv.CV_32FC1, 'maps');

  const def = cv.getDefaultNewCameraMatrix(KP, [W, H], true);
  near(def.data64F[2], W / 2 - 0.5, 1, 'centred principal point');
});

ok('undistortPoints inverts projectPoints', () => {
  const obj = cv.matFromArray(2, 1, cv.CV_64FC3, [0.02, 0.01, 0.5, -0.03, 0.02, 0.5]);
  const zero = cv.matFromArray(3, 1, cv.CV_64F, [0, 0, 0]);
  const img = new cv.Mat(), und = new cv.Mat();

  cv.projectPoints(obj, zero, zero, KP, DP, img);
  cv.undistortPoints(img, und, KP, DP, new cv.Mat(), new cv.Mat(), [3, 20, 1e-9]);
  near(und.data64F[0], 0.02 / 0.5, 1e-4, 'x');
  near(und.data64F[1], 0.01 / 0.5, 1e-4, 'y');
});

ok('cornerSubPix refines in place', () => {
  const img = new cv.Mat(100, 100, cv.CV_8UC1);
  img.setTo(0);
  for(let y = 0; y < 50; y++) for(let x = 0; x < 50; x++) img.data[y * 100 + x] = 255;
  for(let y = 50; y < 100; y++) for(let x = 50; x < 100; x++) img.data[y * 100 + x] = 255;

  const pts = [[49, 49]];
  cv.cornerSubPix(img, pts, [5, 5], [-1, -1], [3 /* TermCriteria EPS + COUNT */, 30, 0.001]);
  assert(pts.length == 1, 'one point back');
  near(pts[0].x !== undefined ? pts[0].x : pts[0][0], 49.5, 1.0, 'x refined near the corner');
});

ok('findCirclesGrid / initCameraMatrix2D / drawFrameAxes are callable', () => {
  assert(typeof cv.findCirclesGrid == 'function' && typeof cv.initCameraMatrix2D == 'function' && typeof cv.drawFrameAxes == 'function');
  assert(typeof cv.calibrateCameraRO == 'function' && typeof cv.stereoCalibrate == 'function' && typeof cv.solvePnPGeneric == 'function' && typeof cv.undistortImagePoints == 'function');
});

ok('calibrateCamera with JS arrays of Mat', () => {
  const objs = [], imgs = [];
  for(let v = 0; v < 6; v++) {
    const rv = cv.matFromArray(3, 1, cv.CV_64F, [(v - 3) * 0.08, (3 - v) * 0.05, 0.02 * v]), tv = cv.matFromArray(3, 1, cv.CV_64F, [0, 0, 0.5 + 0.05 * v]);
    const img = new cv.Mat(), img32 = new cv.Mat();
    cv.projectPoints(objMat, rv, tv, KP, new cv.Mat(), img);
    img.convertTo(img32, cv.CV_32FC2);
    objs.push(cv.matFromArray(54, 1, cv.CV_32FC3, Array.from(objMat.data64F)));
    imgs.push(img32);
  }
  const Kcal = new cv.Mat(), Dcal = new cv.Mat(), rv = [], tv = [];
  const e = cv.calibrateCamera(objs, imgs, [W, H], Kcal, Dcal, rv, tv, 0);

  assert(e < 1, 'rms ' + e);
  assert(rv.length == 6 && tv.length == 6, 'rvecs/tvecs');
});

console.log(failed ? failed + ' FAILED' : 'all passed');
if(failed) throw new Error('fisheye tests failed');

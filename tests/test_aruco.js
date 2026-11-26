import * as cv from 'opencv';

/* ArUco / ChArUco detection round trip: generate a marker (and a board),
 * paint it into a white canvas, detect it back, draw the detection. */

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

const same = (a, b) => JSON.stringify(a) === JSON.stringify(b);
/* OpenCV 5 returns 1-D results as 1xN, 4.x as Nx1: count by rows * cols */
const count = m => m.rows * m.cols;
const idsOf = m => Array.from(m.data32S);

/* pad with a white quiet zone: the detector needs a border around the marker */
function pad(img, n) {
  const out = new cv.Mat();

  cv.copyMakeBorder(img, out, n, n, n, n, cv.BORDER_CONSTANT, new cv.Scalar(255, 255, 255, 255));
  return out;
}

ok('constants and flat names', () => {
  assert(typeof cv.aruco.Dictionary == 'function', 'cv.aruco.Dictionary');
  assert(cv.aruco_Dictionary === cv.aruco.Dictionary, 'aruco_Dictionary alias');
  assert(typeof cv.DICT_4X4_50 == 'number', 'top-level DICT_4X4_50');
  assert(cv.aruco.DICT_6X6_250 == cv.aruco_PredefinedDictionaryType.DICT_6X6_250, 'enum object');
  assert(typeof cv.getPredefinedDictionary == 'function');
});

ok('getPredefinedDictionary / Dictionary properties', () => {
  const d = cv.getPredefinedDictionary(cv.DICT_5X5_100);

  assert(d instanceof cv.aruco.Dictionary, 'instanceof');
  assert(d.markerSize == 5, 'markerSize ' + d.markerSize);
  assert(d.bytesList.rows == 100, 'bytesList rows ' + d.bytesList.rows);
  assert(Object.prototype.toString.call(d) == '[object Dictionary]');
});

ok('generateImageMarker (method and free function)', () => {
  const d = cv.getPredefinedDictionary(cv.DICT_4X4_50);
  const m = d.generateImageMarker(7, 100);
  const m2 = cv.aruco.generateImageMarker(d, 7, 100, 1);

  assert(m.rows == 100 && m.cols == 100, 'size');
  assert(m.type() == cv.CV_8UC1, 'type');
  assert(cv.norm(m, m2) == 0, 'same output');
});

let corners, ids;

ok('ArucoDetector.detectMarkers round trip', () => {
  const d = cv.getPredefinedDictionary(cv.DICT_4X4_50);
  const img = pad(d.generateImageMarker(23, 200), 60);
  const det = new cv.aruco.ArucoDetector(d);
  const r = det.detectMarkers(img);

  assert(count(r.ids) == 1, 'found ' + count(r.ids));
  assert(idsOf(r.ids)[0] == 23, 'id ' + idsOf(r.ids));
  assert(r.corners.length == 1 && r.corners[0].rows * r.corners[0].cols == 4, 'corners');
  corners = r.corners;
  ids = r.ids;

  /* opencv.js style out parameters */
  const c2 = [], i2 = new cv.Mat();

  det.detectMarkers(img, c2, i2);
  assert(c2.length == 1 && idsOf(i2)[0] == 23, 'out parameters');
});

ok('drawDetectedMarkers draws on the image', () => {
  const d = cv.getPredefinedDictionary(cv.DICT_4X4_50);
  const gray = pad(d.generateImageMarker(23, 200), 60);
  const color = new cv.Mat();

  cv.cvtColor(gray, color, cv.COLOR_GRAY2BGR);
  const before = cv.countNonZero(cv.extractChannel ? (() => { const g = new cv.Mat(); cv.extractChannel(color, g, 1); return g; })() : gray);
  cv.drawDetectedMarkers(color, corners, ids);
  cv.aruco.drawDetectedMarkers(color, corners, ids, [0, 255, 0]);

  const g = new cv.Mat();
  cv.extractChannel(color, g, 2);
  const b = new cv.Mat();
  cv.extractChannel(color, b, 0);
  assert(cv.norm(g, b) > 0, 'channels differ after drawing');
});

ok('DetectorParameters / RefineParameters', () => {
  const p = new cv.aruco.DetectorParameters();

  assert(p.adaptiveThreshWinSizeMin == 3, 'default ' + p.adaptiveThreshWinSizeMin);
  p.adaptiveThreshWinSizeMax = 33;
  p.detectInvertedMarker = true;
  p.cornerRefinementMethod = 1;
  assert(p.adaptiveThreshWinSizeMax == 33 && p.detectInvertedMarker === true && p.cornerRefinementMethod == 1, 'set/get');

  const r = new cv.aruco.RefineParameters(5, 2, false);
  assert(r.minRepDistance == 5 && r.errorCorrectionRate == 2 && r.checkAllOrders === false, 'refine');

  const d = cv.getPredefinedDictionary(cv.DICT_4X4_50);
  const det = new cv.aruco.ArucoDetector(d, p, r);
  assert(det.getDetectorParameters().adaptiveThreshWinSizeMax == 33, 'detector keeps params');
});

ok('GridBoard generateImage + detect + matchImagePoints', () => {
  const d = cv.getPredefinedDictionary(cv.DICT_5X5_100);
  const board = new cv.aruco.GridBoard([3, 2], 0.04, 0.01, d);
  const img = board.generateImage([600, 400], 20, 1);
  const det = new cv.aruco.ArucoDetector(d);
  const r = det.detectMarkers(img);

  assert(board instanceof cv.aruco.Board, 'GridBoard instanceof Board');
  assert(board.getGridSize()[0] == 3, 'gridSize');
  assert(count(r.ids) == 6, 'markers found ' + count(r.ids));

  const m = board.matchImagePoints(r.corners, r.ids);
  assert(count(m.objPoints) == 24 && count(m.imgPoints) == 24, 'match points ' + count(m.objPoints));
});

ok('CharucoBoard + CharucoDetector.detectBoard round trip', () => {
  const d = cv.getPredefinedDictionary(cv.DICT_5X5_100);
  const board = new cv.aruco.CharucoBoard([5, 7], 0.04, 0.02, d);
  const img = board.generateImage([500, 700], 20, 1);
  const det = new cv.aruco.CharucoDetector(board);
  const r = det.detectBoard(img);

  assert(board.getChessboardSize()[0] == 5, 'chessboard size');
  assert(count(board.getChessboardCorners()) == 24, 'corners ' + count(board.getChessboardCorners()));
  assert(count(r.charucoIds) == 24, 'charuco corners found ' + count(r.charucoIds));
  assert(count(r.markerIds) == 17, 'markers found ' + count(r.markerIds));

  /* draw */
  const color = new cv.Mat();
  cv.cvtColor(img, color, cv.COLOR_GRAY2BGR);
  cv.drawDetectedCornersCharuco(color, r.charucoCorners, r.charucoIds, [0, 0, 255]);
  cv.drawDetectedMarkers(color, r.markerCorners, r.markerIds);
  const rch = new cv.Mat(), bch = new cv.Mat();
  cv.extractChannel(color, rch, 2);
  cv.extractChannel(color, bch, 0);
  assert(cv.norm(rch, bch) > 0, 'drawn');
});

ok('bad input throws instead of crashing', () => {
  let n = 0;

  try { new cv.aruco.ArucoDetector({}); } catch(e) { n++; }
  try { new cv.aruco.GridBoard([2, 2], 1, 1); } catch(e) { n++; }
  try { new cv.aruco.CharucoDetector({}); } catch(e) { n++; }
  assert(n == 3, 'threw ' + n + '/3');
});

console.log(failed ? failed + ' FAILED' : 'all passed');
if(failed) throw new Error('aruco tests failed');

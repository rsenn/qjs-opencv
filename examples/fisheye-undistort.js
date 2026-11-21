/* Fisheye camera: calibrate once from chessboard photos, then undistort
 * every frame with precomputed maps.
 *
 *   qjsm examples/fisheye-undistort.js calibrate DIR [COLSxROWS] [SQUARE_M] [OUT.json]
 *       reads DIR/*.jpg|png, finds the inner corners (default 9x6, 0.025 m
 *       squares), runs cv.fisheye.calibrate and writes K, D and the image
 *       size to OUT.json (default fisheye.json)
 *
 *   qjsm examples/fisheye-undistort.js undistort CALIB.json [SOURCE] [FRAMES] [BALANCE]
 *       SOURCE is a camera index, a video file or "raspicam" (libcamera,
 *       needs a build with USE_LCCV); the default is camera 0. The maps are
 *       built once, every frame costs one cv.remap. FRAMES > 0 stops after
 *       that many frames and prints the frame rate (no window needed);
 *       BALANCE 0..1 trades field of view (1) against black borders (0).
 *
 *   qjsm examples/fisheye-undistort.js image CALIB.json IN.jpg OUT.png [BALANCE]
 *       undistorts one picture
 *
 * K and D can also be given directly, skipping the calibration: put
 * {"K": [fx,0,cx, 0,fy,cy, 0,0,1], "D": [k1,k2,k3,k4], "size": [w, h]} in the
 * JSON file.
 */
import * as cv from 'opencv';
import * as std from 'std';
import * as os from 'os';

const [mode, ...args] = scriptArgs.slice(1);

function usage() {
  std.err.puts(scriptArgs[0] + ': calibrate DIR [COLSxROWS] [SQUARE_M] [OUT.json] | undistort CALIB.json [SOURCE] [FRAMES] [BALANCE] | image CALIB.json IN OUT [BALANCE]\n');
  std.exit(1);
}

function loadCalibration(file) {
  const c = JSON.parse(std.loadFile(file));

  return { K: cv.matFromArray(3, 3, cv.CV_64F, c.K), D: cv.matFromArray(4, 1, cv.CV_64F, c.D), size: c.size, rms: c.rms };
}

function calibrate(dir, patternArg = '9x6', square = '0.025', out = 'fisheye.json') {
  const [cols, rows] = patternArg.split('x').map(Number);
  const pattern = [cols, rows];
  const squareSize = Number(square);

  /* the board's corners, row by row, on the plane z = 0 */
  const board = [];
  for(let y = 0; y < rows; y++) for(let x = 0; x < cols; x++) board.push(x * squareSize, y * squareSize, 0);
  const boardMat = cv.matFromArray(cols * rows, 1, cv.CV_64FC3, board);

  const [names] = os.readdir(dir);
  const files = names.filter(n => /\.(jpe?g|png|bmp)$/i.test(n)).sort();
  const objectPoints = [], imagePoints = [];
  let size = null;

  for(const name of files) {
    const img = cv.imread(dir + '/' + name);
    const gray = new cv.Mat();

    if(img.empty()) continue;
    cv.cvtColor(img, gray, cv.COLOR_BGR2GRAY);
    size = [gray.cols, gray.rows];

    const corners = new cv.Mat();
    const found = cv.findChessboardCorners(gray, pattern, corners, cv.CALIB_CB_ADAPTIVE_THRESH | cv.CALIB_CB_NORMALIZE_IMAGE | cv.CALIB_CB_FAST_CHECK);

    if(!found) {
      console.log(name, ': no chessboard');
      continue;
    }

    cv.cornerSubPix(gray, corners, [3, 3], [-1, -1], [3 /* TermCriteria EPS + COUNT */, 30, 0.01]);

    const pts = new cv.Mat();
    corners.convertTo(pts, cv.CV_64FC2);
    objectPoints.push(boardMat);
    imagePoints.push(pts);
    console.log(name, ': ok');
  }

  if(objectPoints.length < 5) throw new Error(`need at least 5 usable images, got ${objectPoints.length}`);

  const K = new cv.Mat(), D = new cv.Mat(), rvecs = [], tvecs = [];
  const rms = cv.fisheye.calibrate(objectPoints, imagePoints, size, K, D, rvecs, tvecs, cv.fisheye.CALIB_RECOMPUTE_EXTRINSIC | cv.fisheye.CALIB_FIX_SKEW);

  console.log(`${objectPoints.length} views, rms reprojection error ${rms.toFixed(3)} px`);
  std.writeFile ? std.writeFile(out, '') : 0;

  const f = std.open(out, 'w');
  f.puts(JSON.stringify({ K: Array.from(K.data64F), D: Array.from(D.data64F), size, rms }, null, 2) + '\n');
  f.close();
  console.log('wrote', out);
}

/* the maps for the whole image, built once */
function buildMaps(calib, balance) {
  const R = cv.matFromArray(3, 3, cv.CV_64F, [1, 0, 0, 0, 1, 0, 0, 0, 1]);
  const Knew = new cv.Mat(), map1 = new cv.Mat(), map2 = new cv.Mat();

  cv.fisheye.estimateNewCameraMatrixForUndistortRectify(calib.K, calib.D, calib.size, R, Knew, balance);
  /* CV_16SC2 maps remap faster than CV_32FC1 */
  cv.fisheye.initUndistortRectifyMap(calib.K, calib.D, R, Knew, calib.size, cv.CV_16SC2, map1, map2);
  return { map1, map2 };
}

function openSource(source, size) {
  if(source == 'raspicam') {
    if(!cv.RaspiCam) throw new Error('this build has no cv.RaspiCam (USE_LCCV)');

    const cam = new cv.RaspiCam();
    cam.options.videoWidth = size[0];
    cam.options.videoHeight = size[1];
    cam.startVideo();
    return { read: mat => (cam.getVideoFrame(mat, 1000), !mat.empty()), close: () => cam.stopVideo() };
  }

  const cap = new cv.VideoCapture(/^\d+$/.test(source) ? +source : source);
  if(!cap.isOpened()) throw new Error('cannot open ' + source);
  return { read: mat => cap.read(mat), close: () => cap.release() };
}

function undistortLive(calibFile, source = '0', frames = '0', balance = '0.0') {
  const calib = loadCalibration(calibFile);
  const { map1, map2 } = buildMaps(calib, Number(balance));
  const src = openSource(source, calib.size);
  const frame = new cv.Mat(), out = new cv.Mat();
  const limit = Number(frames);
  let n = 0;
  const t0 = Date.now();

  while(src.read(frame)) {
    cv.remap(frame, out, map1, map2, cv.INTER_LINEAR, cv.BORDER_CONSTANT, new cv.Scalar(0, 0, 0, 0));
    n++;

    if(limit > 0) {
      if(n >= limit) break;
    } else {
      cv.imshow('undistorted', out);
      if(cv.waitKey(1) == 27) break;
    }
  }

  src.close();
  const secs = (Date.now() - t0) / 1000;
  console.log(`${n} frames in ${secs.toFixed(2)} s: ${(n / secs).toFixed(1)} fps (remap + capture)`);
}

function undistortImage(calibFile, input, output, balance = '0.0') {
  const calib = loadCalibration(calibFile);
  const { map1, map2 } = buildMaps(calib, Number(balance));
  const img = cv.imread(input), out = new cv.Mat();

  cv.remap(img, out, map1, map2, cv.INTER_LINEAR, cv.BORDER_CONSTANT, new cv.Scalar(0, 0, 0, 0));
  cv.imwrite(output, out);
}

switch(mode) {
  case 'calibrate': args.length ? calibrate(...args) : usage(); break;
  case 'undistort': args.length ? undistortLive(...args) : usage(); break;
  case 'image': args.length >= 3 ? undistortImage(...args) : usage(); break;
  default: usage();
}

import { HSVtoRGB, HoughCircles, Line, Rect, CV_8UC3, HOUGH_STANDARD, HOUGH_PROBABILISTIC, HOUGH_MULTI_SCALE, HOUGH_GRADIENT_ALT, HOUGH_GRADIENT, drawCircle, threshold, THRESH_BINARY_INV, skeletonization, COLOR_BGR2Lab, COLOR_GRAY2BGR, CV_8UC1, FastLineDetector, LINE_AA, Mat, NORM_MINMAX, cvtColor, equalizeHist, imread, imshow, drawLine, normalize, split, waitKey, } from 'opencv';

function main(arg = 'tests/test_linesegmentdetector.jpg') {
  // Create FLD detector
  // Param               Default value   Description
  // length_threshold    10            - Segments shorter than this will be discarded
  // distance_threshold  1.41421356    - A point placed from a hypothesis line
  //                                     segment farther than this will be
  //                                     regarded as an outlier
  // canny_th1           50            - First threshold for
  //                                     hysteresis procedure in Canny()
  // canny_th2           50            - Second threshold for
  //                                     hysteresis procedure in Canny()
  // canny_aperture_size 3            - Aperturesize for the sobel operator in Canny().
  //                                     If zero, Canny() is not applied and the input
  //                                     image is taken as an edge image.
  // do_merge            false         - If true, incremental merging of segments
  //                                     will be performed
  const length_threshold = 2;
  const distance_threshold = 1.414213562;
  const canny_th1 = 50.0;
  const canny_th2 = 50.0;
  const canny_aperture_size = 0;
  const do_merge = false;
  let lsd = new FastLineDetector(length_threshold, distance_threshold, canny_th1, canny_th2, canny_aperture_size, do_merge);

  const image = imread(arg);

  const gray = Grayscale(image);

  const gray2 = new Mat(gray.size, gray.type);
  const skel = new Mat(gray.size, gray.type);

  threshold(gray, gray2, 100, 255, THRESH_BINARY_INV);

  skeletonization(gray2, skel);

  let lines = new Mat();
  lsd.detect(skel, lines);

  const circles = new Mat();
  HoughCircles(gray, circles, HOUGH_GRADIENT_ALT, 1.5, 30, 100, 0.8);

  console.log('lines', lines.rows);
  //lsd.drawSegments(image, lines, true, [0, 255, 0, 255], 1);

  const canvas = new Mat(image.rows, image.cols * 3, CV_8UC3);

  skeletonization(gray2, skel);
  cvtColor(skel, skel, COLOR_GRAY2BGR);
  cvtColor(gray, gray, COLOR_GRAY2BGR);
  cvtColor(gray2, gray2, COLOR_GRAY2BGR);

  skel.copyTo(canvas(new Rect(0, 0, image.cols, image.rows)));
  gray.copyTo(canvas(new Rect(image.cols, 0, image.cols, image.rows)));
  skel.copyTo(canvas(new Rect(image.cols * 2, 0, image.cols, image.rows)));

  for(let [x, y, r] of circles) {
    console.log(console.config({ compact: true }), { x, y, r });

    drawCircle(canvas, x + image.cols, y, r, [255, 0, 255, 255], 1);
  }

  lines = [...lines].map(l => new Line(...l.map(Math.round)));

  for(let l of lines) drawLine(canvas, l.x1 + image.cols * 2, l.y1, l.x2 + image.cols * 2, l.y2, HSVtoRGB(((l.angle + Math.PI) * 360) / (Math.PI * 2), 1, 1, 255), 1, LINE_AA);

  console.log('lines', lines);

  imshow('test', canvas);
  waitKey(-1);
}

function Grayscale(src) {
  const channels = [];
  const mat = new Mat();
  cvtColor(src, mat, COLOR_BGR2Lab);
  split(mat, channels);
  return channels[0];
}

function Color(src) {
  const mat = new Mat();
  cvtColor(src, mat, COLOR_GRAY2BGR);
  return mat;
}

main(...scriptArgs.slice(1));

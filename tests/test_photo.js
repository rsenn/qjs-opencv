import {
  CV_8UC1,
  INPAINT_TELEA,
  Mat,
  Point,
  Scalar,
  COLOR_BGR2GRAY,
  COLOR_BGRA2BGR,
  cvtColor,
  detailEnhance,
  drawCircle,
  drawLine,
  edgePreservingFilter,
  fastNlMeansDenoising,
  fastNlMeansDenoisingColored,
  imread,
  imshow,
  inpaint,
  pencilSketch,
  stylization,
  waitKey,
} from 'opencv';

function show_wait_destroy(name, mat) {
  imshow(name, mat);
  waitKey(0);
}

function main(filename = 'smarties.png') {
  const src4 = imread(filename);

  if(src4.empty) throw new Error(`Error opening image: ${filename}`);

  /* cv.imread() can return a 4-channel (BGRA) Mat for PNGs with alpha; the
   * photo module's NPR filters (pencilSketch, stylization, detailEnhance,
   * edgePreservingFilter, fastNlMeansDenoisingColored) and inpaint require a
   * 1- or 3-channel image and throw a TypeError otherwise. */
  const src = new Mat();
  cvtColor(src4, src, COLOR_BGRA2BGR);

  show_wait_destroy('source', src);

  const sketchGray = new Mat(),
    sketchColor = new Mat();
  pencilSketch(src, sketchGray, sketchColor, 60, 0.07, 0.02);
  show_wait_destroy('pencilSketch (gray)', sketchGray);
  show_wait_destroy('pencilSketch (color)', sketchColor);

  const stylized = new Mat();
  stylization(src, stylized);
  show_wait_destroy('stylization', stylized);

  const enhanced = new Mat();
  detailEnhance(src, enhanced);
  show_wait_destroy('detailEnhance', enhanced);

  const edgePreserved = new Mat();
  edgePreservingFilter(src, edgePreserved);
  show_wait_destroy('edgePreservingFilter', edgePreserved);

  const denoised = new Mat();
  fastNlMeansDenoisingColored(src, denoised);
  show_wait_destroy('fastNlMeansDenoisingColored', denoised);

  const denoisedGray = new Mat();
  cvtColor(src, denoisedGray, COLOR_BGR2GRAY);
  fastNlMeansDenoising(denoisedGray, denoisedGray);
  show_wait_destroy('fastNlMeansDenoising (gray)', denoisedGray);

  /* inpaint: damage a copy with a scribble, then repair it */
  const damaged = src.clone();
  const mask = new Mat(src.rows, src.cols, CV_8UC1, new Scalar(0));

  drawLine(damaged, new Point(40, 40), new Point(src.cols - 40, src.rows - 40), new Scalar(0, 0, 0), 12);
  drawLine(mask, new Point(40, 40), new Point(src.cols - 40, src.rows - 40), new Scalar(255), 12);
  drawCircle(damaged, new Point(src.cols >> 1, src.rows >> 1), 30, new Scalar(0, 0, 0), -1);
  drawCircle(mask, new Point(src.cols >> 1, src.rows >> 1), 30, new Scalar(255), -1);

  show_wait_destroy('inpaint (damaged)', damaged);

  const repaired = new Mat();
  inpaint(damaged, mask, repaired, 3, INPAINT_TELEA);
  show_wait_destroy('inpaint (repaired)', repaired);
}

main(...scriptArgs.slice(1));

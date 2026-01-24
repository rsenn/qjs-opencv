import { Mat, Scalar, IMREAD_GRAYSCALE, LINE_AA, FastLineDetector, getTickFrequency, getTickCount, Point, drawLine, imread, imshow, waitKey, cvtColor, COLOR_GRAY2BGR } from 'opencv';

function main(input = 'corridor.jpg') {
  const image = imread(input, IMREAD_GRAYSCALE);

  if(image.empty) throw new Error('Unable to open: ' + input);

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
  let length_threshold = 10;
  let distance_threshold = 1.41421356;
  let canny_th1 = 50.0;
  let canny_th2 = 50.0;
  let canny_aperture_size = 3;
  let do_merge = false;
  let fld = new FastLineDetector(length_threshold, distance_threshold, canny_th1, canny_th2, canny_aperture_size, do_merge);
  let lines;

  // Because of some CPU's power strategy, it seems that the first running of
  // an algorithm takes much longer. So here we run the algorithm 5 times
  // to see the algorithm's processing time with sufficiently warmed-up
  // CPU performance.
  for(let run_count = 0; run_count < 5; run_count++) {
    let freq = getTickFrequency();
    lines = new Mat();
    let start = getTickCount();
    // Detect the lines with FLD
    fld.detect(image, lines);
    let duration_ms = ((getTickCount() - start) * 1000) / freq;
    console.log('Elapsed time for FLD ' + duration_ms + ' ms.');
  }

  // Show found lines with FLD
  let line_image_fld = new Mat();
  image.copyTo(line_image_fld);

  cvtColor(line_image_fld, line_image_fld, COLOR_GRAY2BGR);

  //fld.drawSegments(line_image_fld, lines);

  for(let line of lines) drawLine(line_image_fld, line, [0, 0, 255], 1, LINE_AA);

  imshow('FLD result', line_image_fld);
  waitKey();

  return 0;
}

main(...scriptArgs.slice(1));

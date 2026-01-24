import { Mat, imread, resize, Size, IMREAD_COLOR, INTER_LINEAR_EXACT, cvtColor, COLOR_BGR2GRAY, threshold, THRESH_OTSU, THRESH_BINARY_INV, ximgproc, CV_8UC3, mixChannels, Rect, imshow, waitKey, } from 'opencv';

const [me] = scriptArgs;

function main(file = 'opencv-logo.png') {
  const img = imread(file, IMREAD_COLOR);

  resize(img, img, new Size(), 0.5, 0.5, INTER_LINEAR_EXACT);

  console.log(me, img);

  /// Threshold the input image
  const img_grayscale = new Mat(),
    img_binary = new Mat();

  cvtColor(img, img_grayscale, COLOR_BGR2GRAY);
  threshold(img_grayscale, img_binary, 0, 255, THRESH_OTSU | THRESH_BINARY_INV);

  imshow('Binary', img_binary);
  waitKey(-1);

  /// Apply thinning to get a skeleton
  const img_thinning_ZS = new Mat(),
    img_thinning_GH = new Mat();

  console.log(me, { img_thinning_ZS, img_thinning_GH });

  ximgproc.thinning(img_binary, img_thinning_ZS, ximgproc.THINNING_ZHANGSUEN);
  ximgproc.thinning(img_binary, img_thinning_GH, ximgproc.THINNING_GUOHALL);

  imshow('img_thinning', img_thinning_ZS);
  waitKey(-1);
  imshow('img_thinning', img_thinning_GH);
  waitKey(-1);

  /// Make 3 channel images from thinning result
  let result_ZS = new Mat(img.rows, img.cols, CV_8UC3),
    result_GH = new Mat(img.rows, img.cols, CV_8UC3);

  console.log(me, { result_ZS, result_GH });

  const in1 = [img_thinning_ZS, img_thinning_ZS, img_thinning_ZS];
  const in2 = [img_thinning_GH, img_thinning_GH, img_thinning_GH];
  const from_to = [0, 0, 1, 1, 2, 2];

  console.log(me, { in1, in2, from_to });

  mixChannels(in1, 3, [result_ZS], 1, from_to , 3);
  mixChannels(in2, 3, [result_GH], 1, from_to , 3);

  /// Combine everything into a canvas
  const canvas = new Mat(img.rows, img.cols * 3, CV_8UC3);

  console.log(me, { canvas });

  img.copyTo(canvas(new Rect(0, 0, img.cols, img.rows)));
  result_ZS.copyTo(canvas(new Rect(img.cols, 0, img.cols, img.rows)));
  result_GH.copyTo(canvas(new Rect(img.cols * 2, 0, img.cols, img.rows)));

  /// Visualize result
  imshow('Skeleton', canvas);
  waitKey(0);

  return 0;
}

main(...scriptArgs.slice(1));

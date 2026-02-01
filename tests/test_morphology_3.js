import { Size, CV_8UC1, blur, getStructuringElement, MORPH_RECT, IMREAD_COLOR, moveWindow, waitKey, destroyWindow, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, bitwise_not, erode, dilate, imread, imshow, cvtColor, COLOR_BGR2GRAY, adaptiveThreshold, Mat, Point, } from 'opencv';

function main(input = './samples/samples/data/notes.png') {

  const src = imread(input, IMREAD_COLOR);

  if(src.empty) {
    console.log('Could not open or find the image!\n');
    console.log('Usage: ' + scriptArgs[0] + ' <Input image>');
    return -1;
  }

  // Show source image
  imshow('src', src);

  // Transform source image to gray if it is not already
  let gray = new Mat();

  if(src.channels >= 3) {
    cvtColor(src, gray, COLOR_BGR2GRAY);
  } else {
    gray = src;
  }

  // Show gray image
  show_wait_destroy('gray', gray);

  // Apply adaptiveThreshold at the bitwise_not of gray, notice the ~ symbol
  let bw = new Mat(), gray2 = new Mat(gray.size, gray.type);

bitwise_not(gray, gray2);

  /*console.log('gray.size', gray.size);
  console.log('gray.type', gray.type);

  console.log('gray2.size', gray2.size);
  console.log('gray2.channels', gray2.channels);*/

  adaptiveThreshold(gray, bw, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 15, -2);

  // Show binary image
  show_wait_destroy('binary', bw);

  // Create the images that will use to extract the horizontal and vertical lines
  let horizontal = bw.clone();
  let vertical = bw.clone();

  // Specify size on horizontal axis
  let horizontal_size = horizontal.cols / 30;

  // Create structure element for extracting horizontal lines through morphology operations
  let horizontalStructure = getStructuringElement(MORPH_RECT, new Size(horizontal_size, 1));

  // Apply morphology operations
  erode(horizontal, horizontal, horizontalStructure, new Point(-1, -1));
  dilate(horizontal, horizontal, horizontalStructure, new Point(-1, -1));

  // Show extracted horizontal lines
  show_wait_destroy('horizontal', horizontal);

  // Specify size on vertical axis
  let vertical_size = vertical.rows / 30;

  // Create structure element for extracting vertical lines through morphology operations
  let verticalStructure = getStructuringElement(MORPH_RECT, new Size(1, vertical_size));

  // Apply morphology operations
  erode(vertical, vertical, verticalStructure, new Point(-1, -1));
  dilate(vertical, vertical, verticalStructure, new Point(-1, -1));

  // Show extracted vertical lines
  show_wait_destroy('vertical', vertical);

  // Inverse vertical image
  bitwise_not(vertical, vertical);
  show_wait_destroy('vertical_bit', vertical);

  // Extract edges and smooth image according to the logic
  // 1. extract edges
  // 2. dilate(edges)
  // 3. src.copyTo(smooth)
  // 4. blur smooth img
  // 5. smooth.copyTo(src, edges)

  // Step 1
  let edges = new Mat();
  adaptiveThreshold(vertical, edges, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 3, -2);
  show_wait_destroy('edges', edges);

  // Step 2
  let kernel = Mat.ones(2, 2, CV_8UC1);
  dilate(edges, edges, kernel);
  show_wait_destroy('dilate', edges);

  // Step 3
  let smooth = new Mat();
  vertical.copyTo(smooth);

  // Step 4
  blur(smooth, smooth, new Size(2, 2));

  // Step 5
  smooth.copyTo(vertical, edges);

  // Show final result
  show_wait_destroy('smooth - final', vertical);

  return 0;
}

function show_wait_destroy(winname, img) {
  imshow(winname, img);
  moveWindow(winname, 500, 0);
  waitKey(0);
  destroyWindow(winname);
}

main(...scriptArgs.slice(1));

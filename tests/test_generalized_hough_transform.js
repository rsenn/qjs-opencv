import { Mat, imread, IMREAD_GRAYSCALE, cvtColor, COLOR_RGB2GRAY, createGeneralizedHoughBallard, createGeneralizedHoughGuil, Point, Size, Scalar, RotatedRect, imshow, line, waitKey, } from 'opencv';

function main() {
  //  load source image and grayscale template
  const image = imread('tests/generalized_hough_mini_image.jpg');
  const templ = imread('tests/generalized_hough_mini_template.jpg', IMREAD_GRAYSCALE);

  //  create grayscale image
  const grayImage = new Mat();
  cvtColor(image, grayImage, COLOR_RGB2GRAY);

  //  create variable for location, scale and rotation of detected templates
  const positionBallard = new Mat(),
    positionGuil = new Mat();

  //  template width and height
  const w = templ.cols;
  const h = templ.rows;

  //  create ballard and set options
  const ballard = createGeneralizedHoughBallard();
  ballard.setMinDist(10);
  ballard.setLevels(360);
  ballard.setDp(2);
  ballard.setMaxBufferSize(1000);
  ballard.setVotesThreshold(40);

  ballard.setCannyLowThresh(30);
  ballard.setCannyHighThresh(110);
  ballard.setTemplate(templ);

  //  create guil and set options
  const guil = createGeneralizedHoughGuil();
  guil.setMinDist(10);
  guil.setLevels(360);
  guil.setDp(3);
  guil.setMaxBufferSize(1000);

  // XXX: TODO

  /*guil.setMinAngle(0);
  guil.setMaxAngle(360);
  guil.setAngleStep(1);
  guil.setAngleThresh(1500);

  guil.setMinScale(0.5);
  guil.setMaxScale(2.0);
  guil.setScaleStep(0.05);
  guil.setScaleThresh(50);

  guil.setPosThresh(10);*/

  guil.setCannyLowThresh(30);
  guil.setCannyHighThresh(110);

  guil.setTemplate(templ);

  //  execute ballard detection
  ballard.detect(grayImage, positionBallard);
  //  execute guil detection
  guil.detect(grayImage, positionGuil);

  //  draw ballard
  for(let seg of positionBallard) {
    const rRect = new RotatedRect(new Point(seg[0], seg[1]), new Size(w * seg[2], h * seg[2]), seg[3]);

    const vertices = [];
    rRect.points(vertices);

    for(let i = 0; i < 4; i++) line(image, vertices[i], vertices[(i + 1) % 4], Scalar(255, 0, 0), 6);
  }

  //  draw guil
  for(let seg of positionGuil) {
    const rRect = new RotatedRect(new Point(seg[0], seg[1]), new Size(w * seg[2], h * seg[2]), seg[3]);

    const vertices = [];
    rRect.points(vertices);

    for(let i = 0; i < 4; i++) line(image, vertices[i], vertices[(i + 1) % 4], Scalar(0, 255, 0), 2);
  }

  imshow('result_img', image);
  waitKey();
}

main(...scriptArgs.slice(1));

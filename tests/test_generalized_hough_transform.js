import { Mat, imread, cvtColor, COLOR_RGB2GRAY, createGeneralizedHoughBallard, createGeneralizedHoughGuil, Point, Size, Scalar, RotatedRect, imshow, drawLine, waitKey } from 'opencv';

function main() {
  //  load source image and grayscale template
  const image = imread('./samples/samples/cpp/tutorial_code/ImgTrans/generalized_hough_mini_image.jpg');
  const templ = imread('./samples/samples/cpp/tutorial_code/ImgTrans/generalized_hough_mini_template.jpg');
  cvtColor(templ, templ, COLOR_RGB2GRAY);

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
  for(let line of positionBallard) {
    const rRect = new RotatedRect(new Point(line[0], line[1]), new Size(w * line[2], h * line[2]), line[3]);

    const vertices = [];
    rRect.points(vertices);

    for(let i = 0; i < 4; i++) drawLine(image, vertices[i], vertices[(i + 1) % 4], Scalar(255, 0, 0), 6);
  }

  //  draw guil
  for(let line of positionGuil) {
    const rRect = new RotatedRect(new Point(line[0], line[1]), new Size(w * line[2], h * line[2]), line[3]);

    const vertices = [];
    rRect.points(vertices);

    for(let i = 0; i < 4; i++) drawLine(image, vertices[i], vertices[(i + 1) % 4], Scalar(0, 255, 0), 2);
  }

  imshow('result_img', image);
  waitKey();
}

main(...scriptArgs.slice(1));

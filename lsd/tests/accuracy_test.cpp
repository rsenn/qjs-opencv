#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <lsd_opencv.hpp>

using namespace std;

const cv::Size sz(640, 480);

void
checkConstantColor() {
  cv::RNG rng(cv::getTickCount());
  cv::Mat constColor(sz, CV_8UC1, cv::Scalar::all(rng.uniform(0, 256)));

  vector<cv::Vec4i> lines;
  cv::Ptr<cv::LSD> ls = cv::createLSDPtr();
  ls->detect(constColor, lines);

  cv::Mat drawnLines = cv::Mat::zeros(constColor.size(), CV_8UC1);
  ls->drawSegments(drawnLines, lines);
  imshow("checkConstantColor", drawnLines);

  std::cout << "Constant Color - Number of lines: " << lines.size() << " - 0 Wanted." << std::endl;
}

void
checkWhiteNoise() {
  // Generate white noise image
  cv::Mat white_noise(sz, CV_8UC1);
  cv::RNG rng(cv::getTickCount());
  rng.fill(white_noise, cv::RNG::UNIFORM, 0, 256);

  vector<cv::Vec4i> lines;
  cv::Ptr<cv::LSD> ls = cv::createLSDPtr();
  ls->detect(white_noise, lines);

  cv::Mat drawnLines = white_noise.clone(); // cv::Mat::zeros(white_noise.size(), CV_8UC1);
  ls->drawSegments(drawnLines, lines);
  imshow("checkWhiteNoise", drawnLines);

  std::cout << "White Noise    - Number of lines: " << lines.size() << " - 0 Wanted." << std::endl;
}

void
checkRotatedRectangle() {
  cv::RNG rng(cv::getTickCount());
  cv::Mat filledRect = cv::Mat::zeros(sz, CV_8UC1);

  cv::Point center(rng.uniform(sz.width / 4, sz.width * 3 / 4), rng.uniform(sz.height / 4, sz.height * 3 / 4));
  cv::Size rect_size(rng.uniform(sz.width / 8, sz.width / 6), rng.uniform(sz.height / 8, sz.height / 6));
  float angle = rng.uniform(0, 360);

  cv::Point2f vertices[4];

  cv::RotatedRect rRect = cv::RotatedRect(center, rect_size, angle);

  rRect.points(vertices);
  for(int i = 0; i < 4; i++) { cv::line(filledRect, vertices[i], vertices[(i + 1) % 4], cv::Scalar(255), 3); }

  vector<cv::Vec4i> lines;
  cv::Ptr<cv::LSD> ls = cv::createLSDPtr(::LSD_REFINE_ADV);
  ls->detect(filledRect, lines);

  cv::Mat drawnLines = cv::Mat::zeros(filledRect.size(), CV_8UC1);
  ls->drawSegments(drawnLines, lines);
  imshow("checkRotatedRectangle", drawnLines);

  std::cout << "Check Rectangle- Number of lines: " << lines.size() << " - >= 4 Wanted." << std::endl;
}

void
checkLines() {
  cv::RNG rng(cv::getTickCount());
  cv::Mat horzLines(sz, CV_8UC1, cv::Scalar::all(rng.uniform(0, 128)));

  const int numLines = 3;
  for(unsigned int i = 0; i < numLines; ++i) {
    int y = rng.uniform(10, sz.width - 10);
    cv::Point p1(y, 10);
    cv::Point p2(y, sz.height - 10);
    line(horzLines, p1, p2, cv::Scalar(255), 3);
  }

  vector<cv::Vec4i> lines;
  cv::Ptr<cv::LSD> ls = cv::createLSDPtr(::LSD_REFINE_NONE);
  ls->detect(horzLines, lines);

  cv::Mat drawnLines = cv::Mat::zeros(horzLines.size(), CV_8UC1);
  ls->drawSegments(drawnLines, lines);
  imshow("checkLines", drawnLines);

  std::cout << "Lines Check   - Number of lines: " << lines.size() << " - " << numLines * 2 << " Wanted." << std::endl;
}

int
main() {
  checkWhiteNoise();
  checkConstantColor();
  checkRotatedRectangle();
  for(int i = 0; i < 10; ++i) { checkLines(); }
  checkLines();
  cv::waitKey();
  return 0;
}

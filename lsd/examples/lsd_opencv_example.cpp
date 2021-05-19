#include <stdio.h>
#include <stdlib.h>
#include <opencv2/opencv.hpp>

#include "lsd_opencv.hpp"

using namespace std;

#define IMAGE_WIDTH 1280
#define IMAGE_HEIGHT 720

int
main(void) {
  cv::Mat img1(cv::Size(IMAGE_WIDTH / 2, IMAGE_HEIGHT), CV_8UC1, cv::Scalar(255));
  cv::Mat img2(cv::Size(IMAGE_WIDTH / 2, IMAGE_HEIGHT), CV_8UC1, cv::Scalar(0));

  cv::Mat img3(img1.size().height, img1.size().width + img2.size().width, CV_8UC1);
  cv::Mat left(img3, cv::Rect(0, 0, img1.size().width, img1.size().height));
  img1.copyTo(left);
  cv::Mat right(img3, cv::Rect(img1.size().width, 0, img2.size().width, img2.size().height));
  img2.copyTo(right);
  cv::imshow("Image", img3);

  // cv::LSD call
  std::vector<cv::Vec4i> lines;
  std::vector<double> width, prec, nfa;
  cv::Ptr<cv::LSD> ls = cv::createLSDPtr(::LSD_REFINE_ADV);

  double start = double(cv::getTickCount());
  ls->detect(img3, lines, width, prec, nfa);
  double duration_ms = (double(cv::getTickCount()) - start) * 1000 / cv::getTickFrequency();

  std::cout << lines.size() << " line segments found. For " << duration_ms << " ms." << std::endl;
  for(unsigned int i = 0; i < lines.size(); ++i) {
    cout << '\t' << "B: " << lines[i][0] << " " << lines[i][1] << " E: " << lines[i][2] << " " << lines[i][3]
         << " W: " << width[i] << " P:" << prec[i] << " NFA:" << nfa[i] << std::endl;
  }

  cv::waitKey();
  return 0;
}

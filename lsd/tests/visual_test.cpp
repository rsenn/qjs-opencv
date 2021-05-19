#include <iostream>
#include <fstream>
#include <string>

#include "lsd_wrap.hpp"
#include "lsd_opencv.hpp"
#include "opencv2/core/core.hpp"

using namespace std;
using namespace lsdwrap;

int
main(int argc, char** argv) {
  if(argc != 2) {
    std::cout << "visual_test [in]" << std::endl << "\tin - input image" << std::endl;
    return false;
  }

  std::string in = argv[1];

  cv::Mat image = cv::imread(in, cv::IMREAD_GRAYSCALE);
  // imshow("Input image", image);

  //
  // cv::LSD 1.6 test
  //
  LsdWrap lsd_old;
  vector<seg> seg_old;
  double start = double(cv::getTickCount());
  lsd_old.lsdw(image, seg_old);
  // lsd_old.lsd_subdivided(image, seg_old, 3);
  double duration_ms = (double(cv::getTickCount()) - start) * 1000 / cv::getTickFrequency();
  std::cout << "lsd 1.6 - blue\n\t" << seg_old.size() << " line segments found. For " << duration_ms << " ms." << std::endl;

  //
  // OpenCV LSD
  //
  // cv::LSD lsd_cv(LSD_NO_REFINE); // Do not refine lines
  cv::Ptr<cv::LSD> lsd_cv = cv::createLSDPtr();
  vector<cv::Vec4i> lines;

  std::vector<double> width, prec, nfa;
  start = double(cv::getTickCount());

  std::cout << "Before" << std::endl;
  lsd_cv->detect(image, lines);
  std::cout << "After" << std::endl;

  duration_ms = (double(cv::getTickCount()) - start) * 1000 / cv::getTickFrequency();
  std::cout << "OpenCV lsd - red\n\t" << lines.size() << " line segments found. For " << duration_ms << " ms." << std::endl;

  // Copy the old structure to the new
  vector<cv::Vec4i> seg_cvo(seg_old.size());
  for(unsigned int i = 0; i < seg_old.size(); ++i) {
    seg_cvo[i][0] = seg_old[i].x1;
    seg_cvo[i][1] = seg_old[i].y1;
    seg_cvo[i][2] = seg_old[i].x2;
    seg_cvo[i][3] = seg_old[i].y2;
  }

  //
  // Show difference
  //
  cv::Mat drawnLines(image);
  lsd_cv->drawSegments(drawnLines, lines);
  imshow("Drawing segments", drawnLines);

  cv::Mat difference = cv::Mat::zeros(image.size(), CV_8UC3);
  int d = lsd_cv->compareSegments(image.size(), seg_cvo, lines, difference);
  imshow("Segments difference", difference);
  std::cout << "There are " << d << " not overlapping pixels." << std::endl;
  cv::waitKey(0); // wait for human action

  return 0;
}

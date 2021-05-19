#include <iostream>
#include <fstream>
#include <string>

#include "lsd_wrap.hpp"
#include "lsd_opencv.hpp"
#include "opencv2/core/core.hpp"

using namespace std;
using namespace lsdwrap;

const int REPEAT_CYCLE = 10;

int
main(int argc, char** argv) {
  if(argc != 2) {
    std::cout << "perf_test [in]" << std::endl << "\tin - input image" << std::endl;
    return false;
  }

  std::string in = argv[1];

  cv::Mat image = cv::imread(in, cv::IMREAD_GRAYSCALE);
  std::cout << "Input image size: " << image.size() << std::endl;
  std::cout << "Time averaged over " << REPEAT_CYCLE << " iterations." << std::endl;

  //
  // cv::LSD 1.6 test
  //
  LsdWrap lsd_old;
  double start = double(cv::getTickCount());
  for(unsigned int i = 0; i < REPEAT_CYCLE; ++i) {
    vector<seg> seg_old;
    lsd_old.lsdw(image, seg_old);
  }
  double duration_ms = (double(cv::getTickCount()) - start) * 1000 / cv::getTickFrequency();
  std::cout << "lsd 1.6    - " << double(duration_ms) / REPEAT_CYCLE << " ms." << std::endl;

  //
  // OpenCV cv::LSD ADV settings test
  //
  cv::Ptr<cv::LSD> lsd_adv = cv::createLSDPtr(::LSD_REFINE_ADV);
  start = double(cv::getTickCount());
  for(unsigned int i = 0; i < REPEAT_CYCLE; ++i) {
    vector<cv::Vec4i> lines;
    lsd_adv->detect(image, lines);
  }
  duration_ms = (double(cv::getTickCount()) - start) * 1000 / cv::getTickFrequency();
  std::cout << "OpenCV ADV - " << double(duration_ms) / REPEAT_CYCLE << " ms." << std::endl;

  //
  // OpenCV cv::LSD STD settings test
  //
  cv::Ptr<cv::LSD> lsd_std = cv::createLSDPtr(::LSD_REFINE_STD);
  start = double(cv::getTickCount());
  for(unsigned int i = 0; i < REPEAT_CYCLE; ++i) {
    vector<cv::Vec4i> lines;
    lsd_std->detect(image, lines);
  }
  duration_ms = (double(cv::getTickCount()) - start) * 1000 / cv::getTickFrequency();
  std::cout << "OpenCV STD - " << double(duration_ms) / REPEAT_CYCLE << " ms." << std::endl;

  //
  // OpenCV cv::LSD NO refinement settings test
  //
  cv::Ptr<cv::LSD> lsd_no = cv::createLSDPtr(::LSD_REFINE_NONE); // Do not refine lines
  start = double(cv::getTickCount());
  for(unsigned int i = 0; i < REPEAT_CYCLE; ++i) {
    vector<cv::Vec4i> lines;
    lsd_no->detect(image, lines);
  }
  duration_ms = (double(cv::getTickCount()) - start) * 1000 / cv::getTickFrequency();
  std::cout << "OpenCV NO  - " << double(duration_ms) / REPEAT_CYCLE << " ms." << std::endl;

  return 0;
}

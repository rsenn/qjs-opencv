#include <iostream>
#include <string>

#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

#include "lsd_opencv.hpp"

using namespace std;

int
main(int argc, char** argv) {
  if(argc != 2) {
    std::cout << "lsd_lines [input image]" << std::endl;
    return false;
  }

  std::string in = argv[1];

  cv::Mat image = cv::imread(in, cv::IMREAD_GRAYSCALE);

  // Create and cv::LSD detector with std refinement.
  cv::Ptr<cv::LSD> lsd_std = cv::createLSDPtr(::LSD_REFINE_STD);
  double start = double(cv::getTickCount());
  vector<cv::Vec4i> lines_std;
  lsd_std->detect(image, lines_std);
  double duration_ms = (double(cv::getTickCount()) - start) * 1000 / cv::getTickFrequency();
  std::cout << "OpenCV STD (blue) - " << duration_ms << " ms." << std::endl;

  // Create an cv::LSD detector with no refinement applied.
  cv::Ptr<cv::LSD> lsd_none = cv::createLSDPtr(::LSD_REFINE_NONE);
  start = double(cv::getTickCount());
  vector<cv::Vec4i> lines_none;
  lsd_none->detect(image, lines_none);
  duration_ms = (double(cv::getTickCount()) - start) * 1000 / cv::getTickFrequency();
  std::cout << "OpenCV NONE (red)- " << duration_ms << " ms." << std::endl;
  std::cout << "Overlapping pixels are shown in purple." << std::endl;

  cv::Mat difference = cv::Mat::zeros(image.size(), CV_8UC1);
  lsd_none->compareSegments(image.size(), lines_std, lines_none, difference);
  imshow("Line difference", difference);

  cv::Mat drawnLines(image);
  lsd_none->drawSegments(drawnLines, lines_std);
  imshow("Standard refinement", drawnLines);

  cv::waitKey();
  return 0;
}

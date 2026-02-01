#include <iostream>
#include <sstream>
#include <string>
#include <ctime>
#include <cstdio>

#include <opencv2/core.hpp>
#include <opencv2/core/utility.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include "opencv2/objdetect/charuco_detector.hpp"

int
main(int argc, char* argv[]) {

  cv::Matx33d eye = cv::Matx33d::eye();

  std::cout << "eye:" << std::endl << eye << std::endl;

  cv::Matx33d ones = cv::Matx33d::ones();

  std::cout << "ones:" << std::endl << ones << std::endl;

  cv::Matx33d all = cv::Matx33d::all(0.5);

  std::cout << "all(0.5):" << std::endl << all << std::endl;
}

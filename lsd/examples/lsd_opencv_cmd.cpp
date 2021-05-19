#include <iostream>
#include <fstream>
#include <string>
#include <opencv2/opencv.hpp>

#include "lsd_opencv.hpp"

int
main(int argc, char** argv) {
  if(argc != 3) {
    std::cout << "lsd_opencv_cmd [in] [out]" << std::endl
              << "\tin - input image" << std::endl
              << "\tout - output containing a line segment at each line [x1, y1, x2, y2, width, p, -log10(NFA)]" << std::endl;
    return false;
  }

  std::string in = argv[1];
  std::string out = argv[2];

  cv::Mat image = cv::imread(in, cv::IMREAD_GRAYSCALE);

  // cv::LSD call
  std::vector<cv::Vec4i> lines;
  std::vector<double> width, prec;
  cv::Ptr<cv::LSD> lsd = cv::createLSDPtr();

  double start = double(cv::getTickCount());
  lsd->detect(image, lines, width, prec);
  double duration_ms = (double(cv::getTickCount()) - start) * 1000 / cv::getTickFrequency();

  std::cout << lines.size() << " line segments found. For " << duration_ms << " ms." << std::endl;

  cv::cvtColor(image, image, cv::COLOR_GRAY2BGR);

  // Save to file
  std::ofstream segfile;
  segfile.open(out.c_str());
  for(unsigned int i = 0; i < lines.size(); ++i) {
    std::cout << '\t' << "B: " << lines[i][0] << " " << lines[i][1] << " E: " << lines[i][2] << " " << lines[i][3]
              << " W: " << width[i] << " P:" << prec[i] << std::endl;
    segfile << '\t' << "B: " << lines[i][0] << " " << lines[i][1] << " E: " << lines[i][2] << " " << lines[i][3]
            << " W: " << width[i] << " P:" << prec[i] << std::endl;
    cv::line(
        image, cv::Point(lines[i][0], lines[i][1]), cv::Point(lines[i][2], lines[i][3]), cv::Scalar(0, 255, 0), 1, cv::LINE_AA);
  }
  segfile.close();
  cv::imshow(in, image);
  cv::waitKey(-1);
  return 0;
}

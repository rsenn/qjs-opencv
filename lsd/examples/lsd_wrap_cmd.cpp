#include <iostream>
#include <fstream>
#include <string>

#include "lsd_wrap.hpp"

using namespace std;
using namespace lsdwrap;

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

  LsdWrap lsd;
  vector<seg> segments;
  double start = double(cv::getTickCount());

  lsd.lsdw(image, segments);

  double duration_ms = (double(cv::getTickCount()) - start) * 1000 / cv::getTickFrequency();
  std::cout << segments.size() << " line segments found. For " << duration_ms << " ms." << std::endl;

  lsd.imshow_segs(string("Image"), image, segments);

  // Save to file
  ofstream segfile;
  segfile.open(out.c_str());
  vector<seg>::iterator it = segments.begin(), eit = segments.end();
  for(; it != eit; it++) {
    segfile << it->x1 << ' ' << it->y1 << ' ' << it->x2 << ' ' << it->y2 << ' ' << it->width << ' ' << it->p << ' ' << it->NFA
            << std::endl;
  }
  segfile.close();
  cv::waitKey(0);

  return 0;
}

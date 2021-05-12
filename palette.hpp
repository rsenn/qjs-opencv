#ifndef PALETTE_HPP
#define PALETTE_HPP

#include <opencv2/core.hpp>

template<class Pixel>
static inline void
palette_apply(const cv::Mat& src, JSOutputArray dst, Pixel palette[256]) {
  cv::Mat result(src.size(), CV_8UC3);

  printf("result.size() = %ux%u\n", result.cols, result.rows);
  printf("result.channels() = %u\n", result.channels());

  printf("src.elemSize() = %zu\n", src.elemSize());
  printf("result.elemSize() = %zu\n", result.elemSize());
  printf("result.ptr<Pixel>(0,1) - result.ptr<Pixel>(0,0) = %zu\n",
         reinterpret_cast<uchar*>(result.ptr<Pixel>(0, 1)) - reinterpret_cast<uchar*>(result.ptr<Pixel>(0, 0)));

  for(int y = 0; y < src.rows; y++) {
    for(int x = 0; x < src.cols; x++) {
      uchar index = src.at<uchar>(y, x);
      result.at<Pixel>(y, x) = palette[index];
    }
  }

  result.copyTo(dst.getMatRef());
}

#endif /* PALETTE_HPP */

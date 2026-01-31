#ifndef PNG_READ_HPP
#define PNG_READ_HPP

#include "pngpp/reader.hpp"
#include <opencv2/core/mat.hpp>
#include <cstring>

cv::Mat
png_read(const std::string& filename) {
  png::image<png::rgba_pixel> image(filename);

  cv::Size s(image.get_width(), image.get_height());

  // std::cout << "width: " << s.width << " height: " << s.height << std::endl;

  cv::Mat ret(s, CV_8UC4);

  for(size_t i = 0; i < s.height; ++i) {
    for(size_t j = 0; j < s.width; ++j) {
      png::rgba_pixel px = image.get_pixel(j, i);

      int& v = ret.at<int>(i, j);

      v = (px.alpha << 24) | (px.red << 16) | (px.green << 8) | px.blue;
    }
  }

  return ret;
}

#endif /* PNG_READ_HPP */

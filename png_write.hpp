#ifndef PNG_WRITE_HPP
#define PNG_WRITE_HPP

#include <opencv2/core.hpp>
//#include <png++/png.hpp>
#include "pngpp/png.hpp"
#include <vector>

template<class ColorType>
static inline void
write_mat(const std::string& filename, const cv::Mat& mat, const std::vector<ColorType>& palette) {

  png::image<png::index_pixel> image(mat.cols, mat.rows);

  png::palette pal(palette.size());
  size_t i = 0;

  for(const ColorType& color : palette) {
    pal[i] = png::color(color.r, color.g, color.b);
    i++;
  }

  image.set_palette(pal);

  for(png::uint_32 y = 0; y < mat.rows; ++y) {
    for(png::uint_32 x = 0; x < mat.cols; ++x) {
      auto index = mat.at<uchar>(y, x);
      // if(index > 0) std::cout << x << "," << y << ": " << (int)index << std::endl;
      image[y][x] = png::index_pixel(index);
    }
  }

  image.write(filename);
}

#endif /* PNG_WRITE_HPP */

#ifndef PNG_WRITE_HPP
#define PNG_WRITE_HPP

#include "pngpp/color.hpp"
#include "pngpp/image.hpp"
#include "pngpp/index_pixel.hpp"
#include "pngpp/palette.hpp"
#include "pngpp/pixel_buffer.hpp"
#include "pngpp/types.hpp"
#include <ext/alloc_traits.h>
#include <opencv2/core/hal/interface.h>
#include <stddef.h>
#include <fstream>
#include <sstream>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/mat.inl.hpp>
#include <string>
#include <vector>

template<class ColorType>
static inline png::image<png::index_pixel>
png_create(const cv::Mat& mat, const std::vector<ColorType>& palette) {

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

  return image;
}

template<class ColorType>
void
png_write(const std::string& filename, const cv::Mat& mat, const std::vector<ColorType>& palette) {

  /*  std::ofstream stream(filename, std::ios::binary);
         stream.exceptions(std::ios::badbit);*/
  auto image = png_create(mat, palette);

  image.write(filename);
}

template<class ColorType>
std::string
png_write(const cv::Mat& mat, const std::vector<ColorType>& palette) {
  std::ostringstream os;
  auto image = png_create(mat, palette);
  image.write_stream(os);
  return os.str();
}

#endif /* PNG_WRITE_HPP */

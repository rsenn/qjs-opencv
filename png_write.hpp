
#ifndef PNG_WRITE_HPP
#define PNG_WRITE_HPP

#include <opencv2/core/hal/interface.h>  // for uchar
#include <opencv2/core/mat.hpp>          // for Mat
#include <opencv2/core/mat.inl.hpp>      // for Mat::at
#include "pngpp/color.hpp"               // for color
#include "pngpp/image.hpp"               // for image
#include "pngpp/index_pixel.hpp"         // for index_pixel
#include "pngpp/palette.hpp"             // for palette
#include "pngpp/pixel_buffer.hpp"        // for basic_pixel_buffer<>::row_type
#include "pngpp/types.hpp"               // for uint_32
#include <ext/alloc_traits.h>            // for __alloc_traits<>::value_type
#include <stddef.h>                      // for size_t
#include <fstream>                       // for ostringstream
#include <string>                        // for string
#include <vector>                        // for vector
//#include <png++/png.hpp>

template<class ColorType>
static inline png::image<png::index_pixel>
create_image(const cv::Mat& mat, const std::vector<ColorType>& palette) {

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
write_mat(const std::string& filename, const cv::Mat& mat, const std::vector<ColorType>& palette) {

  /*  std::ofstream stream(filename, std::ios::binary);
         stream.exceptions(std::ios::badbit);*/
  auto image = create_image(mat, palette);

  image.write(filename);
}

template<class ColorType>
std::string
write_mat(const cv::Mat& mat, const std::vector<ColorType>& palette) {
  std::ostringstream os;
  auto image = create_image(mat, palette);
  image.write_stream(os);
  return os.str();
}

#endif /* PNG_WRITE_HPP */

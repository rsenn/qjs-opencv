#ifndef GIF_WRITE_HPP
#define GIF_WRITE_HPP

#include "gifenc/gifenc.h"
#include <ext/alloc_traits.h>
#include <opencv2/core/hal/interface.h>
#include <stddef.h>
#include <fstream>
#include <sstream>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/mat.inl.hpp>
#include <string>
#include <vector>

/*template<class ColorType>
static inline ge_GIF *
create_image(const cv::Mat& mat, const std::vector<ColorType>& palette) {

  gif::image<gif::index_pixel> image(mat.cols, mat.rows);

  gif::palette pal(palette.size());
  size_t i = 0;

  for(const ColorType& color : palette) {
    pal[i] = gif::color(color.r, color.g, color.b);
    i++;
  }

  image.set_palette(pal);

  for(gif::uint_32 y = 0; y < mat.rows; ++y) {
    for(gif::uint_32 x = 0; x < mat.cols; ++x) {
      auto index = mat.at<uchar>(y, x);
       image[y][x] = gif::index_pixel(index);
    }
  }

  return image;
}*/

template<class ColorType>
void
write_mat(const std::string& filename, const cv::Mat& mat, const std::vector<ColorType>& palette) {

ge_GIF *gif = ge_new_gif(filename.c_str(), mat.cols, mat.rows, reinterpret_cast<uint8_t*>( palette.data() ), 8, 0, -1);

/*
  auto image = create_image(mat, palette);

  image.write(filename);*/
}

/*template<class ColorType>
std::string
write_mat(const cv::Mat& mat, const std::vector<ColorType>& palette) {
  std::ostringstream os;
  auto image = create_image(mat, palette);
  image.write_stream(os);
  return os.str();
}*/

#endif /* GIF_WRITE_HPP */

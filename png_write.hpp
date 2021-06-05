
#ifndef PNG_WRITE_HPP
#define PNG_WRITE_HPP

#include <opencv2/core.hpp>
//#include <png++/png.hpp>
#include "pngpp/png.hpp"
#include <vector>
#include <fstream>
#include <sstream>

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

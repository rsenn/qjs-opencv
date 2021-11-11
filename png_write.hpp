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

typedef png::image<png::index_pixel> IndexedPNG;

static inline IndexedPNG
png_new(const cv::Size& size) {
  return IndexedPNG(size.width, size.height);
}

template<class ColorType>
static inline void
png_set_palette(IndexedPNG& img, const std::vector<ColorType>& pal) {
  png::palette palette(pal.size());
  size_t i = 0;

  for(const ColorType& pix : pal) {
    palette[i] = png::color(pix.r, pix.g, pix.b);
    i++;
  }

  img.set_palette(palette);
}

static inline void
png_set_pixels(IndexedPNG& img, const cv::Mat& mat) {
  size_t w = img.get_width(), h = img.get_height();

  for(png::uint_32 y = 0; y < h; ++y) {
    for(png::uint_32 x = 0; x < w; ++x) {
      auto index = mat.at<uchar>(y, x);
      img[y][x] = png::index_pixel(index);
    }
  }
}

static inline void
png_set_transparency(IndexedPNG& img, png::byte transparent_index) {
  png::tRNS t(1);
  t[0] = transparent_index;
  img.set_tRNS(t);
}

template<class ColorType>
static inline IndexedPNG
png_create(const cv::Mat& mat, const std::vector<ColorType>& pal, int trans = -1) {
  IndexedPNG img = png_new(mat.size());

  png_set_palette(img, pal);

  if(trans >= 0 && trans <= 255)
    png_set_transparency(img, trans);

  png_set_pixels(img, mat);

  return img;
}

template<class ColorType>
void
png_write(const std::string& filename, const cv::Mat& mat, const std::vector<ColorType>& pal, int trans = -1) {
  IndexedPNG img = png_create(mat, pal, trans);

  img.write(filename);
}

template<class ColorType>
std::string
png_write(const cv::Mat& mat, const std::vector<ColorType>& pal, int trans = -1) {
  std::ostringstream os;
  IndexedPNG img = png_create(mat, pal, trans);

  img.write_stream(os);
  return os.str();
}

#endif /* PNG_WRITE_HPP */

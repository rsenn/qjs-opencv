#ifndef GIF_WRITE_HPP
#define GIF_WRITE_HPP

#include "gifenc/gifenc.h"
#include "util.hpp"
#include "jsbindings.hpp"
#include "palette.hpp"
#include <opencv2/core.hpp>
#include <vector>
#include <algorithm>

namespace {
using std::sort;
using std::string;
using std::transform;
using std::vector;

static inline void
gif_write_image(ge_GIF* gif, const cv::Mat& indexed) {
  size_t index = 0;
  uint16_t width = gif->w, height = gif->h;

  for(uint16_t y = 0; y < height; y++) {
    for(uint16_t x = 0; x < width; x++) {
      uchar pixel = indexed.at<uchar>(y, x);

      gif->frame[index++] = pixel;
    }
  }
}

template<class ColorType>
void
gif_write(const string& file, const vector<cv::Mat>& mats, const vector<int>& delays, const vector<ColorType>& palette, int transparent = -1, int loop = 0) {
  size_t i, n, size, depth;

  vector<uint8_t> pal;

  depth = ceil(log2(palette.size()));
  size = pow(2, depth);
  n = palette.size();
  pal.resize(size * 3);

  for(i = 0; i < size; i++) {
    pal[i * 3 + 0] = i < n ? palette[i].r : 0;
    pal[i * 3 + 1] = i < n ? palette[i].g : 0;
    pal[i * 3 + 2] = i < n ? palette[i].b : 0;
  }

  size_t frames = mats.size();
  vector<size_t> widths, heights;

  widths.resize(frames);
  heights.resize(frames);

  transform(mats.begin(), mats.end(), widths.begin(), [](const cv::Mat& mat) -> size_t { return mat.cols; });
  transform(mats.begin(), mats.end(), heights.begin(), [](const cv::Mat& mat) -> size_t { return mat.rows; });

  sort(widths.begin(), widths.end());
  sort(heights.begin(), heights.end());

  size_t w = widths[0];
  size_t h = heights[0];

  ge_GIF* gif = ge_new_gif(file.c_str(), w, h, reinterpret_cast<uint8_t*>(pal.data()), depth, transparent, loop);
  size_t frame = 0;

  for(const cv::Mat& mat : mats) {

    cv::Mat indexed(mat.size(), CV_8UC1);

    if(mat.channels() == 1 && mat.depth() == 0)
      mat.copyTo(indexed);
    else
      palette_match(mat, indexed, palette, mat.channels() > 3 ? transparent : -1);

    if(frame > 0)
      ge_add_frame(gif, delays[(frame - 1) % delays.size()]);

    gif_write_image(gif, indexed);

    frame++;
  }

  ge_close_gif(gif);
}
} // namespace
#endif /* GIF_WRITE_HPP */

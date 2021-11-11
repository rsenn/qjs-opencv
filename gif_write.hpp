#ifndef GIF_WRITE_HPP
#define GIF_WRITE_HPP

#include "gifenc/gifenc.h"
#include "util.hpp"
#include <opencv2/core.hpp>
#include <vector>
#include <algorithm>

template<class ColorType>
void
gif_write(const std::string& filename,
          const std::vector<cv::Mat>& mats,
          const std::vector<int>& delays,
          const std::vector<ColorType>& palette,
          int transparent = -1,
          int loop = 0) {
  size_t i, n, size, depth;

  std::vector<std::array<uint8_t, 3>> pal;

  depth = ceil(log2(palette.size()));
  size = pow(2, depth);
  n = palette.size();
  pal.resize(size);

  for(i = 0; i < n; i++) {
    pal[i][0] = palette[i].r;
    pal[i][1] = palette[i].g;
    pal[i][2] = palette[i].b;
  }

  size_t frames = mats.size();
  std::vector<size_t> widths, heights;

  widths.resize(frames);
  heights.resize(frames);

  std::transform(mats.begin(), mats.end(), widths.begin(), [](const cv::Mat& mat) -> size_t { return mat.cols; });
  std::transform(mats.begin(), mats.end(), heights.begin(), [](const cv::Mat& mat) -> size_t { return mat.rows; });

  std::sort(widths.begin(), widths.end());
  std::sort(heights.begin(), heights.end());

  size_t w = widths[0];
  size_t h = heights[0];

  ge_GIF* gif = ge_new_gif(filename.c_str(), w, h, reinterpret_cast<uint8_t*>(pal.data()), depth, transparent, loop);
  size_t frame = 0;

  for(const cv::Mat& mat : mats) {
    size_t x, y, index = 0;

    if(frame > 0)
      ge_add_frame(gif, delays[(frame - 1) % delays.size()]);

    for(x = 0; x < w; x++) {
      for(y = 0; y < h; y++) {
        uint8_t pixel = mat_at<uchar>(mat, y, x);

        gif->frame[index++] = pixel;
      }
    }
    frame++;
  }
  ge_close_gif(gif);
}

#endif /* GIF_WRITE_HPP */

#ifndef PIXEL_NEIGHBORHOOD_HPP
#define PIXEL_NEIGHBORHOOD_HPP

#include <opencv2/core/hal/interface.h>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/mat.inl.hpp>
#include <opencv2/core/types.hpp>
#include <stddef.h>
#include <vector>
#include <array>
#include <algorithm>

using std::array;
using std::count_if;
using std::transform;
using std::vector;

template<class Callable, size_t N>
static cv::Mat
pixel_offset_pred(const cv::Mat& src, array<int32_t, N> const& offsets, Callable pred) {
  cv::Mat dst(src.size(), CV_8UC1);
  int32_t step = src.step;
  uchar const* row = src.ptr<uchar>(1, 1);

  for(int y = 1; y < src.rows - 1; y++) {
    for(int x = 1; x < src.cols - 1; x++) {
      uchar const* pixel = row + x;
      array<uchar, 8> p;

      transform(offsets.begin(), offsets.end(), p.begin(), [pixel](int32_t offset) -> uchar { return pixel[offset]; });

      *dst.ptr<uchar>(y, x) = pred(src.at<uchar>(y, x), count_if(p.begin(), p.end(), [](uchar value) -> bool { return value > 0; }));
    }

    row += step;
  }

  return dst;
}

template<class Callable>
static inline cv::Mat
pixel_neighborhood_pred(const cv::Mat& src, Callable pred) {
  int32_t step = src.step;
  array<int32_t, 8> const offsets = {
      -step - 1,
      -step,
      -step + 1,
      -1,
      +1,
      step - 1,
      step,
      step + 1,
  };

  return pixel_offset_pred(src, offsets, pred);
}

template<class Callable>
static inline cv::Mat
pixel_neighborhood_cross_pred(const cv::Mat& src, Callable pred) {
  int32_t step = src.step;
  array<int32_t, 4> const offsets = {
      -step,
      -1,
      +1,
      step,
  };

  return pixel_offset_pred(src, offsets, pred);
}

static vector<cv::Point>
pixel_find_value(const cv::Mat& mat, uchar value) {
  vector<cv::Point> result;

  for(int y = 0; y < mat.rows; y++)
    for(int x = 0; x < mat.cols; x++)
      if(mat.at<uchar>(y, x) == value)
        result.push_back(cv::Point(x, y));

  return result;
}

template<class Callable, size_t N>
static cv::Mat
pixel_neighborhood_pred2(const cv::Mat& src, array<int32_t, N> const& offsets, Callable pred) {
  cv::Mat dst(src.size(), CV_8UC1);
  /*int32_t step = src.step;
  uchar const* row = src.ptr<uchar>(1, 1);*/

  for(int y = 1; y < src.rows - 2; y++) {
    for(int x = 1; x < src.cols - 2; x++) {
      /*uchar const* pixel = row + x;*/
      array<uchar, 8> p;

      transform(offsets.begin(), offsets.end(), p.begin(), [src, y, x](int32_t offset) -> uchar { return *(src.ptr<uchar>(y, x) + offset); });

      std::cout << p <<std::endl;

      *dst.ptr<uchar>(y, x) = pred(src.at<uchar>(y, x), count_if(p.begin(), p.end(), [](uchar value) -> bool { return value > 0; }));
    }

    // row += step;
  }

  return dst;
}

static inline cv::Mat
pixel_neighborhood(const cv::Mat& mat) {
  return pixel_neighborhood_pred2(mat,
                                  (array<int32_t, 8> const){
                                      -mat.step - 1,
                                      -mat.step,
                                      -mat.step + 1,
                                      -1,
                                      +1,
                                      mat.step - 1,
                                      mat.step,
                                      mat.step + 1,
                                  },
                                  [](uchar value, uchar count) -> uchar { return value ? count : 0; });
}

static inline cv::Mat
pixel_neighborhood_if(const cv::Mat& mat, uchar match_count) {
  return pixel_neighborhood_pred(mat, [match_count](uchar value, uchar count) -> uchar { return count == match_count ? 0xff : 0; });
}

static inline cv::Mat
pixel_neighborhood_cross(const cv::Mat& mat) {
  return pixel_neighborhood_cross_pred(mat, [](uchar value, uchar count) -> uchar { return value ? count : 0; });
}

static inline cv::Mat
pixel_neighborhood_cross_if(const cv::Mat& mat, uchar match_count) {
  return pixel_neighborhood_cross_pred(mat, [match_count](uchar value, uchar count) -> uchar { return count == match_count ? 0xff : 0; });
}

#endif /* PIXEL_NEIGHBORHOOD_HPP */

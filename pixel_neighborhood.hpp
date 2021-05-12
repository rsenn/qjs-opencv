#ifndef PIXEL_NEIGHBORHOOD_HPP
#define PIXEL_NEIGHBORHOOD_HPP

#include <opencv2/core.hpp>
#include <functional>

static uchar
pixel_neighborhood_default_pred(uchar value, uchar count) {
  return value ? count : 0;
}

template<class Callable>
static cv::Mat
pixel_neighborhood_cross_pred(const cv::Mat& mat, Callable pred) {
  cv::Mat result(mat.size(), CV_8UC1);
  size_t row_size = mat.ptr<uchar>(1, 0) - mat.ptr<uchar>(0, 0);

  for(int y = 1; y < mat.rows - 1; y++) {
    for(int x = 1; x < mat.cols - 1; x++) {

      uchar count = (mat.at<uchar>(y - 1, x) > 0) + (mat.at<uchar>(y, x + 1) > 0) + (mat.at<uchar>(y + 1, x) > 0) +
                    (mat.at<uchar>(y, x - 1) > 0);

      result.at<uchar>(y, x) = pred(mat.at<uchar>(y, x), count);
    }
  }
  return result;
}

template<class Callable>
static cv::Mat
pixel_neighborhood_pred(const cv::Mat& mat, Callable pred) {
  cv::Mat result(mat.size(), CV_8UC1);

  for(int y = 1; y < mat.rows - 1; y++) {
    for(int x = 1; x < mat.cols - 1; x++) {
      uchar p2 = mat.at<uchar>(y - 1, x);
      uchar p3 = mat.at<uchar>(y - 1, x + 1);
      uchar p4 = mat.at<uchar>(y, x + 1);
      uchar p5 = mat.at<uchar>(y + 1, x + 1);
      uchar p6 = mat.at<uchar>(y + 1, x);
      uchar p7 = mat.at<uchar>(y + 1, x - 1);
      uchar p8 = mat.at<uchar>(y, x - 1);
      uchar p9 = mat.at<uchar>(y - 1, x - 1);

      uchar count = (p2 > 0) + (p3 > 0) + (p4 > 0) + (p5 > 0) + (p6 > 0) + (p7 > 0) + (p8 > 0) + (p9 > 0);

      result.at<uchar>(y, x) = pred(mat.at<uchar>(y, x), count);
    }
  }
  return result;
}

static std::vector<cv::Point>
pixel_find_value(const cv::Mat& mat, uchar value) {
  std::vector<cv::Point> result;

  for(int y = 0; y < mat.rows; y++) {
    for(int x = 0; x < mat.cols; x++) {
      if(mat.at<uchar>(y, x) == value)
        result.push_back(cv::Point(x, y));
    }
  }

  return result;
}

class MatchCount {
public:
  MatchCount(uchar value) : m_value(value) {}

  uchar
  operator()(uchar value, uchar count) {
    if(value)
      return count == m_value ? 0xff : 0;
    return 0;
  }

private:
  uchar m_value;
};

static inline cv::Mat
pixel_neighborhood(const cv::Mat& mat) {
  return pixel_neighborhood_pred(mat, pixel_neighborhood_default_pred);
}

static inline cv::Mat
pixel_neighborhood_if(const cv::Mat& mat, uchar match_count) {
  return pixel_neighborhood_pred(mat, MatchCount(match_count));
}

static inline cv::Mat
pixel_neighborhood_cross(const cv::Mat& mat) {
  return pixel_neighborhood_cross_pred(mat, pixel_neighborhood_default_pred);
}

static inline cv::Mat
pixel_neighborhood_cross_if(const cv::Mat& mat, uchar match_count) {
  return pixel_neighborhood_cross_pred(mat, MatchCount(match_count));
}

#endif /* PIXEL_NEIGHBORHOOD_HPP */

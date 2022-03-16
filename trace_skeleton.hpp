#ifndef TRACE_SKELETON_HPP
#define TRACE_SKELETON_HPP

#include "pixel_neighborhood.hpp"

using cv::Mat;
using cv::Point;
using std::make_pair;
using std::pair;
using std::vector;

template<typename T = bool(uchar)>
static bool
pixel_find_nonnull(
    cv::Point& pt, const cv::Mat& mat, cv::Mat& already_taken, T pred = [](uchar p) -> bool { return p > 0; }) {
  int max_y = mat.rows, max_x = mat.cols;
  for(int y = 0; y < max_y; y++) {
    for(int x = 0; x < max_x; x++) {
      uchar* taken = already_taken.ptr<uchar>(y, x);
      if(*taken == 0 && pred(mat.at<uchar>(y, x))) {
        pt = cv::Point(x, y);
        *taken = 0xff;
        return true;
      }
    }
  }
  return false;
}

template<typename T = bool(uchar)>
static std::pair<bool, cv::Point>
pixel_find_nonnull(
    const cv::Mat& mat, cv::Mat& already_taken, T pred = [](uchar p) -> bool { return p > 0; }) {
  std::pair<bool, cv::Point> result = std::make_pair<bool, cv::Point>(false, cv::Point(-1, -1));

  result.first = pixel_find_nonnull(result.second, mat, already_taken, pred);

  return result;
}

static inline bool
pixel_check(int y, int x, std::vector<cv::Point>& out, const cv::Mat& mat, cv::Mat& already_taken) {
  uchar* taken = already_taken.ptr<uchar>(y, x);

  if(mat.at<uchar>(y, x) && *taken == 0) {
    out.push_back(cv::Point(x, y));
    *taken = 0xff;
    return true;
  }

  return false;
}

template<typename T = bool(uchar)>
static inline size_t
pixel_get_neighbours(
    std::vector<cv::Point>& result, const cv::Point& pt, const cv::Mat& mat, cv::Mat& already_taken, T pred = [](uchar p) -> bool { return p > 0; }) {

  result.clear();

  if(!pixel_check(pt.y - 1, pt.x, result, mat, already_taken))
    if(!pixel_check(pt.y, pt.x + 1, result, mat, already_taken))
      if(!pixel_check(pt.y + 1, pt.x, result, mat, already_taken))
        if(!pixel_check(pt.y, pt.x - 1, result, mat, already_taken))
          if(!pixel_check(pt.y - 1, pt.x + 1, result, mat, already_taken))
            if(!pixel_check(pt.y + 1, pt.x + 1, result, mat, already_taken))
              if(!pixel_check(pt.y + 1, pt.x - 1, result, mat, already_taken))
                pixel_check(pt.y - 1, pt.x - 1, result, mat, already_taken);

  return result.size();
}

template<typename T = bool(uchar)>
static inline std::vector<cv::Point>
pixel_get_neighbours(
    const cv::Point& pt, const cv::Mat& mat, cv::Mat& already_taken, T pred = [](uchar p) -> bool { return p > 0; }) {
  std::vector<cv::Point> result;
  pixel_get_neighbours(result, pt, mat, already_taken, pred);
  return result;
}

static inline void
trace_skeleton(const cv::Mat& mat, JSContoursData<double>& contours) {
  cv::Mat bitmap(mat.rows, mat.cols, CV_8UC1);
  cv::Mat neighbors(mat.rows, mat.cols, CV_8UC1);
  cv::Point pt;
  std::vector<cv::Point> adjacent;

  neighbors = pixel_neighborhood(mat);

  while(pixel_find_nonnull(pt, neighbors, bitmap)) {

    contours.push_back(JSContourData<double>());

    JSContourData<double>& contour = contours.back();
    for(;;) {
      contour.push_back(pt);

      if(pixel_get_neighbours(adjacent, pt, mat, bitmap) == 0)
        break;

      pt = adjacent.front();
    }
  }
}

static inline JSContoursData<double>
trace_skeleton(const cv::Mat& mat) {
  JSContoursData<double> contours;
  trace_skeleton(mat, contours);
  return contours;
}

#endif /* TRACE_SKELETON_HPP */

#ifndef TRACE_SKELETON_HPP
#define TRACE_SKELETON_HPP

#include "pixel_neighborhood.hpp"

namespace skeleton_tracing {

using cv::Mat;
using cv::Point;
using cv::Point_;
using std::vector;

template<typename T>
static inline Point_<T>
point_difference(const Point_<T>& a, const Point_<T>& b) {
  return Point_<T>(b.x - a.x, b.y - a.y);
}

template<typename T>
static inline Point_<T>
point_sum(const Point_<T>& a, const Point_<T>& b) {
  return Point_<T>(a.x + b.x, a.y + b.y);
}

template<typename T>
static inline bool
point_equal(const Point_<T>& a, const Point_<T>& b) {
  return a.x == b.x && a.y == b.y;
}

template<typename M = Mat>
static inline bool
point_inside(int y, int x, const M& mat) {
  return x >= 0 && x < mat.cols && y >= 0 && y < mat.rows;
}

template<typename T, typename M = Mat>
static inline bool
point_inside(const Point_<T>& pt, const M& mat) {
  return point_inside(pt.y, pt.x, mat);
}

static inline int32_t&
pixel_taken(int y, int x, Mat& already_taken) {
  return *already_taken.ptr<int32_t>(y, x);
}

template<typename T = bool(uchar)>
static bool
pixel_find_nonnull(Point& pt, const Mat& mat, Mat& already_taken, int32_t index, T pred) {
  int h = mat.rows, w = mat.cols;
  for(int y = 0; y < h; y++) {
    for(int x = 0; x < w; x++) {
      int32_t& taken = pixel_taken(y, x, already_taken);
      if(taken == -1 && pred(mat.at<uchar>(y, x))) {
        pt = Point(x, y);
        taken = index;
        return true;
      }
    }
  }
  return false;
}

template<typename T = bool(uchar)>
static inline bool
pixel_check(int y, int x, vector<Point>& out, const Mat& mat, Mat& already_taken, int32_t index, T pred) {

  if(!point_inside(y, x, mat))
    return false;

  int32_t& taken = pixel_taken(y, x, already_taken);

  if(pred(mat.at<uchar>(y, x)) && taken == -1) {
    out.push_back(Point(x, y));
    taken = index;
    return true;
  }

  return false;
}

template<typename T = bool(uchar)>
static inline bool
pixel_check(int y, int x, Point& out, const Mat& mat, Mat& already_taken, int32_t index, T pred) {

  if(!point_inside(y, x, mat))
    return false;

  int32_t& taken = pixel_taken(y, x, already_taken);

  if(pred(mat.at<uchar>(y, x)) && taken == -1) {
    out = Point(x, y);
    taken = index;
    return true;
  }

  return false;
}

template<typename T = bool(uchar)>
static bool
pixel_get_neighbours(vector<Point>& r, const Point& pt, const Mat& mat, Mat& taken, int32_t index, T pred) {

  r.clear();

  if(pixel_check(pt.y - 1, pt.x, r, mat, taken, index, pred))
    return true;
  if(pixel_check(pt.y, pt.x + 1, r, mat, taken, index, pred))
    return true;
  if(pixel_check(pt.y + 1, pt.x, r, mat, taken, index, pred))
    return true;
  if(pixel_check(pt.y, pt.x - 1, r, mat, taken, index, pred))
    return true;
  if(pixel_check(pt.y - 1, pt.x + 1, r, mat, taken, index, pred))
    return true;
  if(pixel_check(pt.y + 1, pt.x + 1, r, mat, taken, index, pred))
    return true;
  if(pixel_check(pt.y + 1, pt.x - 1, r, mat, taken, index, pred))
    return true;
  if(pixel_check(pt.y - 1, pt.x - 1, r, mat, taken, index, pred))
    return true;

  return false;
}

template<typename T = bool(uchar)>
static inline bool
pixel_get_neighbour(Point& r, const Point& pt, const Point& off, const Mat& mat, Mat& taken, int32_t index, T pred) {
  return pixel_check(pt.y + off.y, pt.x + off.x, r, mat, taken, index, pred);
}

template<typename T = bool(uchar)>
static inline vector<Point>
pixel_get_neighbours(const Point& pt, const Mat& mat, Mat& already_taken, T pred) {
  vector<Point> result;
  pixel_get_neighbours(result, pt, mat, already_taken, pred);
  return result;
}

static inline void
run(const Mat& mat, JSContoursData<double>& contours) {
  Mat bitmap(mat.rows, mat.cols, CV_32SC1);
  Mat neighbors(mat.rows, mat.cols, CV_8UC1);
  Point pt;
  vector<Point> adjacent;
  size_t index;

  bitmap ^= int32_t(-1);

  neighbors = pixel_neighborhood(mat);

  const auto pred = [](uchar p) -> bool { return p > 0; };

  for(index = 0; pixel_find_nonnull(pt, neighbors, bitmap, index, pred); ++index) {
    size_t count;
    Point prev, diff, next;

    contours.push_back(JSContourData<double>());

    JSContourData<double>& contour = contours.back();
    for(count = 0;; ++count, pt = next) {
      contour.push_back(pt);

      if(count > 0 && pixel_get_neighbour(next, pt, prev, mat, bitmap, index, pred)) {

      } else {
        if(pixel_get_neighbours(adjacent, pt, mat, bitmap, index, pred) == 0)
          break;
        next = adjacent.front();
      }

      diff = point_difference(pt, next);

      if(count > 0) {
        if(point_equal(prev, diff))
          contour.pop_back();
      }

      prev = diff;
    }
  }
}
}; // namespace skeleton_tracing

static inline void
trace_skeleton(const cv::Mat& mat, JSContoursData<double>& out) {
  skeleton_tracing::run(mat, out);
}

static inline JSContoursData<double>
trace_skeleton(const cv::Mat& mat) {
  JSContoursData<double> contours;
  skeleton_tracing::run(mat, contours);
  return contours;
}

#endif /* TRACE_SKELETON_HPP */

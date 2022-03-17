#ifndef TRACE_SKELETON_HPP
#define TRACE_SKELETON_HPP

#include "pixel_neighborhood.hpp"

namespace skeleton_tracing {

using cv::Mat;
using cv::Point;
using std::vector;

static inline int32_t&
pixel_taken(const Point& pt, Mat& mapping) {
  return *mapping.ptr<int32_t>(pt.y, pt.x);
}

template<typename T = uchar>
static inline T
pixel_at(const Point& pt, const Mat& mat) {
  return mat.at<T>(pt.y, pt.x);
}

template<typename Predicate = bool(unsigned)>
static inline bool
pixel_find_nonnull(Point& out, const Mat& mat, const Mat& neighborhood, Mat& mapping, int32_t index, Predicate pred) {
  int h = mat.rows, w = mat.cols;
  Point pt;
  for(pt.y = 0; pt.y < h; pt.y++) {
    for(pt.x = 0; pt.x < w; pt.x++) {
      int32_t& taken = pixel_taken(pt, mapping);
      if(taken == -1 && pixel_at(pt, neighborhood) > 0 && pred(pixel_at(pt, mat))) {
        out = pt;
        taken = index;
        return true;
      }
    }
  }
  return false;
}

template<typename Predicate = bool(unsigned)>
static inline bool
pixel_check(const Point& pt, const Mat& mat, Mat& mapping, int32_t index, Predicate pred, Point& out) {

  if(!point_inside(pt, mat))
    return false;

  int32_t& taken = pixel_taken(pt, mapping);
  if(taken == -1 && pred(pixel_at(pt, mat))) {
    out = pt;
    taken = index;
    return true;
  }

  return false;
}

template<typename Predicate = bool(unsigned)>
static bool
pixel_neighbour(const Point& pt, const Mat& mat, Mat& taken, int32_t index, Predicate pred, Point& r) {

  static const Point points[8] = {
      Point(0, -1),
      Point(1, 0),
      Point(0, 1),
      Point(-1, 0),
      Point(1, -1),
      Point(1, 1),
      Point(-1, 1),
      Point(-1, -1),
  };

  for(const auto& offs : points)
    if(pixel_check(point_sum(pt, offs), mat, taken, index, pred, r))
      return true;

  return false;
}

static inline void
run(const Mat& mat, JSContoursData<double>& contours, bool simplify = false) {
  Mat mapping(mat.rows, mat.cols, CV_32SC1);
  Mat neighborhood = pixel_neighborhood(mat);
  Point pt;

  mapping ^= int32_t(-1);

  const auto pred = [](unsigned p) -> bool { return p > 0; };

  for(size_t index = 0; pixel_find_nonnull(pt, mat, neighborhood, mapping, index, pred); ++index) {
    size_t n;
    Point ppdiff, pdiff, diff, next;

    contours.push_back(JSContourData<double>());
    JSContourData<double>& contour = contours.back();

    for(n = 0;; ++n, pt = next) {
      contour.push_back(pt);

      if(!(n > 0 && pixel_check(point_sum(pt, pdiff), mat, mapping, index, pred, next)))
        if(!(n > 1 && pixel_check(point_sum(pt, ppdiff), mat, mapping, index, pred, next)))
          if(!pixel_neighbour(pt, mat, mapping, index, pred, next))
            break;

      diff = point_difference(pt, next);

      if(simplify) {
        if(n > 0 && point_equal(pdiff, diff))
          contour.pop_back();
      }

      ppdiff = pdiff;
      pdiff = diff;
    }
  }
}
}; // namespace skeleton_tracing

static inline void
trace_skeleton(const cv::Mat& mat, JSContoursData<double>& out, bool simplify = false) {
  skeleton_tracing::run(mat, out, simplify);
}

static inline JSContoursData<double>
trace_skeleton(const cv::Mat& mat, bool simplify = false) {
  JSContoursData<double> contours;
  skeleton_tracing::run(mat, contours, simplify);
  return contours;
}

#endif /* TRACE_SKELETON_HPP */

#ifndef TRACE_SKELETON_HPP
#define TRACE_SKELETON_HPP

#include "pixel_neighborhood.hpp"

namespace skeleton_tracing {

using cv::Mat;
using cv::Point;
using std::vector;

static const std::array<Point, 8> direction_points = {
    Point(0, 1),
    Point(1, 1),
    Point(1, 0),
    Point(1, -1),
    Point(0, -1),
    Point(-1, -1),
    Point(-1, 0),
    Point(-1, 1),
};

template<typename T>
static inline T&
pixel_ref(const Point& pt, Mat& mat) {
  return *mat.ptr<T>(pt.y, pt.x);
}

template<typename T = uchar>
static inline T
pixel_at(const Point& pt, const Mat& mat) {
  return mat.at<T>(pt.y, pt.x);
}

static inline void
pixel_remove(const Point& pt, Mat& mat) {
  uchar& value = pixel_ref<uchar>(pt, mat);

  if(value > 0)
    value = 0;
}

static inline Point
point_direction(int dir) {
  static const size_t n = countof(direction_points);

  dir = ((dir % n) + n) % n;
  return direction_points[dir];
}

static inline int
point_direction(const Point& delta) {
  const int lut[3][3] = {
      {5, 4, 3},
      {6, -1, 2},
      {7, 0, 1},
  };

  Point pt = point_normalize<double>(delta);

  return lut[delta.y + 1][delta.x + 1];
}

static inline void
points_direction(int dir, int range, vector<Point>& points) {

  points.push_back(point_direction(dir));

  if(range > 0 && points.size() < 7) {
    points_direction(dir - 1, range - 1, points);
    points_direction(dir + 1, range - 1, points);
  }
}

class skeleton_tracer {
public:
  explicit skeleton_tracer(Mat& _m) : mat(_m), mapping(_m.rows, _m.cols, CV_32SC1), neighborhood(pixel_neighborhood(_m)), index(0) { mapping.setTo(-1); }

  void
  decrement_neighborhood(const Point& pt) {
    for(const auto& v : direction_points) {
      Point q = point_sum(pt, v);

      if(q.y >= 0 && q.y < neighborhood.rows) {
        if(q.x >= 0 && q.x < neighborhood.cols) {
          uchar& value = pixel_ref<uchar>(q, neighborhood);

          if(value > 0)
            value -= 1;
        }
      }
    }
  }

  template<typename Predicate = bool(unsigned)>
  bool
  pixel_find_pred(Point& out, int32_t index, Predicate pred) {
    int h = mat.rows, w = mat.cols;
    Point pt;

    for(pt.y = 0; pt.y < h; pt.y++) {
      for(pt.x = 0; pt.x < w; pt.x++) {
        int& taken = pixel_ref<int>(pt, mapping);

        if(taken == -1 && pixel_at(pt, neighborhood) > 0 && pred(pixel_at(pt, mat))) {
          out = pt;
          taken = index;
          decrement_neighborhood(pt);
          // pixel_remove(pt, mat);
          return true;
        }
      }
    }

    return false;
  }

  template<typename Predicate = bool(unsigned)>
  bool
  pixel_check(const Point& pt, int32_t index, Predicate pred, Point& out) {
    if(!point_inside(pt, mat))
      return false;

    int& taken = pixel_ref<int>(pt, mapping);

    if(taken == -1 && pred(pixel_at(pt, mat))) {
      out = pt;
      taken = index;
      decrement_neighborhood(pt);
      return true;
    }

    return false;
  }

  template<typename Predicate = bool(unsigned)>
  bool
  pixel_neighbour(const Point& pt, int32_t index, Predicate pred, Point& r, const vector<Point>& points) {
    for(const auto& offs : points)
      if(pixel_check(point_sum(pt, offs), index, pred, r))
        return true;

    return false;
  }

  template<typename Predicate = bool(unsigned)>
  bool
  trace(JSContoursData<double>& contours, bool simplify, Predicate pred) {
    Point pt, ppdiff, pdiff, diff, next;
    size_t n;
    int dir = -1;
    vector<Point> points;

    if(!pixel_find_pred(pt, index, pred))
      return false;

    contours.push_back(JSContourData<double>());
    JSContourData<double>& contour = contours.back();

    points.resize(direction_points.size());
    std::copy(direction_points.begin(), direction_points.end(), points.begin());

    // points_direction(0, 4, points);

    for(n = 0;; ++n, pt = next) {
      contour.push_back(pt);

      if(!(n > 0 && pixel_check(point_sum(pt, pdiff), index, pred, next)))
        if(!(n > 1 && pixel_check(point_sum(pt, ppdiff), index, pred, next)))

          if(/*n != 0 ||*/ !pixel_neighbour(pt, index, pred, next, points))
            break;

      diff = point_difference(pt, next);

      if((diff.x || diff.y)) {
        // std::cout << index << ": [" << n << "] direction " << point_direction(diff) << std::endl;

        if(dir == -1) {
          dir = point_direction(diff);
          points.clear();
          points_direction(dir, 1, points);
        }
      }
      if(simplify) {
        if(n > 0 && point_equal(pdiff, diff))
          contour.pop_back();
      }

      ppdiff = pdiff;
      pdiff = diff;
    }

    ++index;
    return true;
  }

  uint32_t
  run(JSContoursData<double>& contours, bool simplify = false) {

    while(trace(contours, simplify, [](unsigned p) -> bool { return p > 0; })) {
#ifdef DEBUG_OUTPUT
      std::cout << index << ": [" << contours.back().size() << "]" << std::endl;
#endif
    }

    return index;
  }

private:
  int32_t index;
  Mat& mat;

public:
  Mat mapping, neighborhood;
};
}; // namespace skeleton_tracing

static inline uint32_t
trace_skeleton(cv::Mat& mat, JSContoursData<double>& out, cv::Mat* neighborhood, cv::Mat* mapping, bool simplify = false) {
  uint32_t ret;
  skeleton_tracing::skeleton_tracer tracer(mat);

  if(neighborhood)
    tracer.neighborhood.copyTo(*neighborhood);

  ret = tracer.run(out, simplify);

  if(mapping)
    tracer.mapping.copyTo(*mapping);

  return ret;
  // return skeleton_tracing::run(mat, out, simplify);
}

static inline uint32_t
trace_skeleton(cv::Mat& mat, JSContoursData<double>& out, bool simplify = false) {
  skeleton_tracing::skeleton_tracer tracer(mat);
  return tracer.run(out, simplify);
  // return skeleton_tracing::run(mat, out, simplify);
}

static inline JSContoursData<double>
trace_skeleton(cv::Mat& mat, bool simplify = false) {
  JSContoursData<double> contours;
  trace_skeleton(mat, contours, simplify);
  return contours;
}

#endif /* TRACE_SKELETON_HPP */

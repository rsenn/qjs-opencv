#include "line.hpp"
#include "psimpl.hpp"

float
point_distance(const cv::Point2f& p1, const cv::Point2f& p2) {
  return std::sqrt(psimpl::math::point_distance2<2>(&p1.x, &p2.x));
}

double
point_distance(const cv::Point2d& p1, const cv::Point2d& p2) {
  return std::sqrt(psimpl::math::point_distance2<2>(&p1.x, &p2.x));
}

int
point_distance(const cv::Point& p1, const cv::Point& p2) {
  return std::sqrt(psimpl::math::point_distance2<2>(&p1.x, &p2.x));
}
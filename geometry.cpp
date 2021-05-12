#include "geometry.hpp"
#include <opencv2/imgproc/imgproc.hpp>

point_vector<float>
get_mass_centers(std::vector<point_vector<int>> contours) {
  std::vector<cv::Moments> mu(contours.size());
  point_vector<float> mc(contours.size());
  for(size_t i = 0; i < contours.size(); i++) mu[i] = cv::moments(contours[i], false);
  for(size_t i = 0; i < contours.size(); i++) mc[i] = cv::Point2f(mu[i].m10 / mu[i].m00, mu[i].m01 / mu[i].m00);

  return mc;
}

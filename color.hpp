#ifndef COLOR_HPP
#define COLOR_HPP

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/types_c.h>

#include "simple_svg_1.0.0.hpp"

typedef cv::Scalar color_type;

/*
 * H(Hue): 0 - 360 degree (integer)
 * S(Saturation): 0 - 1.00 (double)
 * V(Value): 0 - 1.00 (double)
 *
 * output[3]: Output, std::array size 3, int
 */
color_type hsv_to_rgb(int H, double S, double V);

inline svg::Color
from_scalar(const color_type& s) {
  return svg::Color(s[0], s[1], s[2]);
}

#endif // defined COLOR_HPP

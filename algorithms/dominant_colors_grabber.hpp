#ifndef DOMINANT_COLORS_GRABBER_HPP
#define DOMINANT_COLORS_GRABBER_HPP

#pragma once
#include <vector>
#include <opencv2/core.hpp>

enum color_space { CS_UNDEFINED = -1, CS_BGR = 0, CS_HSV };

enum dist_type { DT_UNDEFINED = -1, DT_CUBE = 0, DT_CIE76, DT_CIE94, DT_KMEANS };

#define DOM_COLORS_COUNT_DEFAULT 12
#define DOM_COLORS_PART_DEFAULT 95 // colors % of whole image
#define DOM_COLORS_COLOR_SPACE_DEFAULT color_space::CS_BGR
#define DOM_COLORS_DIST_TYPE_DEFAULT dist_type::DT_CUBE

#define COLOR_HEIGHT_DEFAULT 20

class dominant_colors_grabber {
public:
  explicit dominant_colors_grabber(color_space cs = DOM_COLORS_COLOR_SPACE_DEFAULT,
                                   dist_type dt = DOM_COLORS_DIST_TYPE_DEFAULT,
                                   unsigned colors_count = DOM_COLORS_COUNT_DEFAULT,
                                   double colors_part = DOM_COLORS_PART_DEFAULT);
  std::vector<cv::Scalar>
  GetDomColors(cv::Mat img, color_space cs = CS_UNDEFINED, dist_type dt = DT_UNDEFINED, unsigned colors_count = 0, double colors_part = 0);

  void SetDistanceType(dist_type dt);
  dist_type GetDistanceType();

  void SetColorSpace(color_space cs);
  color_space GetColorSpace();

  void SetColorsCount(unsigned colors_count);
  unsigned GetColorsCount();

  void SetColorsPart(double colors_part);
  double GetColorsPart();

  void SetParam(cv::Vec3i param);
  cv::Vec3i GetParam();

protected:
  unsigned _colors_count;
  double _colors_part;
  color_space _cs;
  dist_type _dt;
  cv::Vec3i _param;
};

cv::Mat GetHist(cv::Mat img, color_space cs);
template<class val_type> void DrawCube(cv::Mat img, cv::Point3i center, cv::Vec3i size, val_type value = 0, std::vector<bool> cyclic = {0, 0, 0});
template<class val_type> void DrawCube(cv::Mat img, cv::Vec3i p1, cv::Vec3i p2, val_type value = 0, std::vector<bool> cyclic = {0, 0, 0});
void MarkNearColors(cv::Mat mask, cv::Point3i center, cv::Vec3f size, unsigned char value, color_space cs, dist_type dt);
void MarkNearColorsCIE(cv::Mat mask, cv::Point3i color, double dist, unsigned char value, color_space cs, dist_type dt);
double GetCIE76Dist(cv::Vec3i c1, cv::Vec3i c2, color_space cs);
double GetCIE94Dist(cv::Vec3i c1, cv::Vec3i c2, color_space cs);

cv::Vec4f GetCenter(cv::Mat img, cv::Mat w_mask, cv::Mat v_mask = cv::Mat());
std::vector<cv::Vec3i> GetGabarits(cv::Point3i center, cv::Vec3i size);

template<class val_type> val_type CycleRange(val_type val, val_type val1, val_type val2);
void CyclePoint3d(cv::Vec3i& p, cv::MatSize size);

cv::Mat ShowColors(cv::Mat img, std::vector<cv::Scalar> colors, unsigned color_height = COLOR_HEIGHT_DEFAULT);

extern const std::vector<int> hist_sizes[2];
extern const std::vector<float> hist_ranges[2];

#endif // defined(DOMINANT_COLORS_GRABBER_HPP)

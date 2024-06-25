#include "dominant_colors_grabber.hpp"
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>

#pragma warning(disable : 4244 4267)

dominant_colors_grabber::dominant_colors_grabber(color_space cs, dist_type dt, unsigned colors_count, double colors_part)
    : _cs(cs), _dt(dt), _colors_count(colors_count), _colors_part(colors_part) {
}

std::vector<cv::Scalar>
dominant_colors_grabber::GetDomColors(cv::Mat img, color_space cs, dist_type dt, unsigned colors_count, double colors_part) {
  if(cs == CS_UNDEFINED)
    cs = _cs;

  if(dt == DT_UNDEFINED)
    dt = _dt;

  if(colors_count == 0)
    colors_count = _colors_count;

  if(colors_part == 0)
    colors_part = _colors_part;

  std::vector<cv::Scalar> res;

  if(dt == DT_KMEANS) {
    cv::Mat img_cs;

    switch(cs) {
      case CS_HSV: cv::cvtColor(img, img_cs, cv::COLOR_BGR2HSV); break;
      case CS_BGR: img.copyTo(img_cs);
    }

    cv::Mat img_samples = img_cs.reshape(1, img.cols * img.rows);

    img_samples.convertTo(img_samples, CV_32F);
    cv::TermCriteria term_crit(cv::TermCriteria::EPS | cv::TermCriteria::COUNT, 5, 1.0);
    cv::Mat labels, colors_mat;
    cv::kmeans(img_samples, colors_count, labels, term_crit, 5, cv::KMEANS_PP_CENTERS, colors_mat);
    colors_mat = colors_mat.reshape(3);
    res.resize(colors_mat.rows);

    for(unsigned i = 0; i < colors_mat.rows; i++) {
      res[i] = colors_mat.at<cv::Vec3f>(i, 0);
    }
  } else {
    cv::Mat hist = GetHist(img, cs);
    cv::Scalar full_w = sum(hist);
    hist /= full_w[0];
    double sum = 0;
    cv::Mat hist_mask(3, hist.size, CV_8UC1, cv::Scalar(255));
    cv::Mat center_mask = hist_mask.clone();

    std::vector<float> koefs = {hist_ranges[cs][1] / hist_sizes[cs][0], hist_ranges[cs][3] / hist_sizes[cs][1], hist_ranges[cs][5] / hist_sizes[cs][2]};

    while((res.size() < colors_count) && (sum <= colors_part / 100.0)) {
      center_mask *= 0;
      int max_pos[3] = {0, 0, 0};
      double max_val;
      cv::minMaxIdx(hist, NULL, &max_val, NULL, max_pos, hist_mask);
      cv::Point3i max_loc(max_pos[0], max_pos[1], max_pos[2]);
      // getting center and sum
      MarkNearColors(center_mask, max_loc, _param, 255, cs, dt);      // mark near
      cv::Vec4f center_sum = GetCenter(hist, center_mask, hist_mask); // get center of near-maximum area
      sum += center_sum[3];
      center_sum[0] *= koefs[0];
      center_sum[1] *= koefs[1];
      center_sum[2] *= koefs[2];
      res.push_back(center_sum);
      // exclude near-maximum cells
      center_mask = 255 - center_mask;
      bitwise_and(hist_mask, center_mask, hist_mask);
    }
  }

  return res;
}

#pragma region SET / GET
void
dominant_colors_grabber::SetDistanceType(dist_type dt) {
  if(dt != DT_UNDEFINED)
    _dt = dt;
}

dist_type
dominant_colors_grabber::GetDistanceType() {
  return _dt;
}

void
dominant_colors_grabber::SetColorSpace(color_space cs) {
  if(cs != CS_UNDEFINED)
    _cs = cs;
}

color_space
dominant_colors_grabber::GetColorSpace() {
  return _cs;
}

void
dominant_colors_grabber::SetColorsCount(unsigned colors_count) {
  if(colors_count != 0)
    _colors_count = colors_count;
}

unsigned
dominant_colors_grabber::GetColorsCount() {
  return _colors_count;
}

void
dominant_colors_grabber::SetColorsPart(double colors_part) {
  if(colors_part > 0)
    _colors_part = colors_part;
}

double
dominant_colors_grabber::GetColorsPart() {
  return _colors_part;
}

void
dominant_colors_grabber::SetParam(cv::Vec3i param) {
  _param = param;
}

cv::Vec3i
dominant_colors_grabber::GetParam() {
  return _param;
}

#pragma endregion
void
MarkNearColors(cv::Mat mask, cv::Point3i center, cv::Vec3f size, unsigned char value, color_space cs, dist_type dt) {
  std::vector<bool> cyclic_dims = {1, 0, 0}; // hue channel is cyclic

  switch(cs) {
    case CS_BGR: cyclic_dims[0] = false; break;
  }

  switch(dt) {
    case DT_CIE76:
    case DT_CIE94: MarkNearColorsCIE(mask, center, size[0], value, cs, dt); break;
    case DT_CUBE: DrawCube<unsigned char>(mask, center, size, value, cyclic_dims); break;
  }
}

void
MarkNearColorsCIE(cv::Mat mask, cv::Point3i color, double dist, unsigned char value, color_space cs, dist_type dt) {
  cv::Vec3i center_color;
  std::vector<float> koefs = {hist_ranges[cs][1] / hist_sizes[cs][0], hist_ranges[cs][3] / hist_sizes[cs][1], hist_ranges[cs][5] / hist_sizes[cs][2]};
  center_color[0] = color.x * koefs[0];
  center_color[1] = color.y * koefs[1];
  center_color[2] = color.z * koefs[2];

  for(int i1 = 0; i1 < mask.size[0]; i1++)
    for(int i2 = 0; i2 < mask.size[1]; i2++)
      for(int i3 = 0; i3 < mask.size[2]; i3++) {
        cv::Vec3i cl(i1, i2, i3);
        cl[0] *= koefs[0];
        cl[1] *= koefs[1];
        cl[2] *= koefs[2];
        cv::Vec3i p(i1, i2, i3);
        double color_dist;
        switch(dt) {
          case DT_CIE76: color_dist = GetCIE76Dist(cl, center_color, cs); break;
          case DT_CIE94: color_dist = GetCIE94Dist(cl, center_color, cs); break;
        }
        if(color_dist < dist) {
          mask.at<unsigned char>(p) = value;
        }
      }
}

double
GetCIE76Dist(cv::Vec3i c1, cv::Vec3i c2, color_space cs) {
  double res = 0;
  cv::Mat colors(1, 2, CV_8UC3);
  colors.at<cv::Vec3b>(0, 0) = c1;
  colors.at<cv::Vec3b>(0, 1) = c2;

  switch(cs) {
    case CS_HSV: cv::cvtColor(colors, colors, cv::COLOR_HSV2BGR); break;
  }

  cv::cvtColor(colors, colors, cv::COLOR_BGR2Lab);
  c1 = colors.at<cv::Vec3b>(0, 0);
  c2 = colors.at<cv::Vec3b>(0, 1);
  res = cv::norm(c1 - c2);

  return res;
}

double
GetCIE94Dist(cv::Vec3i c1, cv::Vec3i c2, color_space cs) {
  double res = 0;
  cv::Mat colors(1, 2, CV_8UC3);

  colors.at<cv::Vec3b>(0, 0) = c1;
  colors.at<cv::Vec3b>(0, 1) = c2;

  switch(cs) {
    case CS_HSV: cv::cvtColor(colors, colors, cv::COLOR_HSV2BGR); break;
  }

  double D76 = GetCIE76Dist(c1, c2, cs);
  cv::cvtColor(colors, colors, cv::COLOR_BGR2Lab);
  c1 = colors.at<cv::Vec3b>(0, 0);
  c2 = colors.at<cv::Vec3b>(0, 1);
  double dL = c1[0] - c2[0];
  double C1 = sqrt(c1[1] * c1[1] + c1[2] * c1[2]);
  double C2 = sqrt(c2[1] * c2[1] + c2[2] * c2[2]);
  double dC = C1 - C2;
  double dH = sqrt(D76 * D76 - dL * dL - dC * dC);
  double k1 = 0.045, k2 = 0.015;
  double kC, kH, kL;
  kC = kH = kL = 1;
  double SL = 1, SC = 1 + k1 * C1, SH = 1 + k2 * C1;
  res = sqrt(pow(dL / (kL * SL), 2) + pow(dC / (kC * SC), 2) + pow(dH / (kH * SH), 2));

  return res;
}

cv::Mat
GetHist(cv::Mat img, color_space cs) {
  cv::Mat img_colors;
  std::vector<int> channels = {0, 1, 2};

  switch(cs) {
    case CS_BGR: {
      img.copyTo(img_colors);
      break;
    }

    case CS_HSV: {
      cvtColor(img, img_colors, cv::COLOR_BGR2HSV);
      break;
    }
  }

  std::vector<cv::Mat> img_channels;
  cv::split(img_colors, img_channels);
  cv::Mat color_hist;
  cv::calcHist(img_channels, channels, cv::Mat(), color_hist, hist_sizes[cs], hist_ranges[cs]); // 3D histogram

  return color_hist;
}

template<class val_type>
void
DrawCube(cv::Mat img, cv::Point3i center, cv::Vec3i size, val_type value, std::vector<bool> cyclic) {
  std::vector<cv::Vec3i> p = GetGabarits(center, size);

  return DrawCube<val_type>(img, p[0], p[1], value, cyclic);
}

template<class val_type>
void
DrawCube(cv::Mat img, cv::Vec3i p1, cv::Vec3i p2, val_type value, std::vector<bool> cyclic) {
  for(unsigned i = 0; i < cyclic.size(); i++) // cut cubes if non cyclic
    if(!cyclic[i]) {
      p1[i] = cv::max(0, p1[i]);
      p2[i] = cv::min(img.size[i] - 1, p2[i]);
    }

  for(int i1 = p1[0]; i1 <= p2[0]; i1++)
    for(int i2 = p1[1]; i2 <= p2[1]; i2++)
      for(int i3 = p1[2]; i3 <= p2[2]; i3++) {
        cv::Vec3i p(i1, i2, i3);
        CyclePoint3d(p, img.size);
        img.at<val_type>(p) = value; // uchar because of mask
      }
}

cv::Vec4f
GetCenter(cv::Mat img, cv::Mat w_mask, cv::Mat v_mask) {
  cv::Vec4f res(0, 0, 0, 0);
  cv::Vec3f center(0, 0, 0);
  float sum = 0;

  for(int i1 = 0; i1 < img.size[0]; i1++)
    for(int i2 = 0; i2 < img.size[1]; i2++)
      for(int i3 = 0; i3 < img.size[2]; i3++) {
        cv::Vec3i p(i1, i2, i3);
        if(w_mask.at<unsigned char>(p) == 0)
          continue;
        float val = img.at<float>(p);
        if(!v_mask.empty() && v_mask.at<unsigned char>(p)) {
          res[3] += val;
        } else
          val = 0;
        cv::Vec3f pf = p;
        center += pf * val;
        sum += val;
      }

  center /= sum;
  res[0] = center[0];
  res[1] = center[1];
  res[2] = center[2];

  return res;
}

std::vector<cv::Vec3i>
GetGabarits(cv::Point3i center, cv::Vec3i size) {
  std::vector<cv::Vec3i> res = {center, center};

  res[0][0] -= size[0];
  res[0][1] -= size[1];
  res[0][2] -= size[2];
  res[1][0] += size[0];
  res[1][1] += size[1];
  res[1][2] += size[2];

  return res;
}

template<class val_type>
val_type
CycleRange(val_type val, val_type val1, val_type val2) {
  val_type ranged_val = (val - val1) % (val2 - val1);

  if(ranged_val < 0)
    ranged_val += (val2 - val1);

  return ranged_val + val1;
}

void
CyclePoint3d(cv::Vec3i& p, cv::MatSize size) {
  for(unsigned i = 0; i < 3; i++)
    p[i] = CycleRange(p[i], 0, size[i]);
}

cv::Mat
ShowColors(cv::Mat img, std::vector<cv::Scalar> colors, unsigned color_height) {
  cv::Mat img_colors;
  cv::copyMakeBorder(img, img_colors, color_height, 0, 0, 0, cv::BORDER_CONSTANT, cv::Scalar::all(0));
  cv::Rect2d color_rect(cv::Point(0, 0), cv::Size((double)img_colors.cols / colors.size(), color_height));

  for(unsigned i = 0; i < colors.size(); i++) {
    cv::rectangle(img_colors, color_rect, colors[i], cv::FILLED);
    color_rect.x += color_rect.width;
  }

  return img_colors;
}

const std::vector<int> hist_sizes[2] = {{16, 16, 16}, {18, 8, 8}};
const std::vector<float> hist_ranges[2] = {{0, 256, 0, 256, 0, 256}, {0, 180, 0, 256, 0, 256}};

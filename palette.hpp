#ifndef PALETTE_HPP
#define PALETTE_HPP

#include "jsbindings.hpp"
#include "js_array.hpp"
#include <opencv2/core/hal/interface.h>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/mat.inl.hpp>
#include <cstdio>

template<class Container>
static inline void
palette_read(JSContext* ctx, JSValueConst arr, Container& out, float factor = 1.0f) {
  size_t i, len = js_array_length(ctx, arr);

  for(i = 0; i < len; i++) {
    cv::Scalar scalar;
    JSColorData<uint8_t> color;
    JSValue item = JS_GetPropertyUint32(ctx, arr, i);
    js_array_to(ctx, item, scalar);
    JS_FreeValue(ctx, item);

    std::transform(&scalar[0], &scalar[4], color.arr.begin(), [factor](double val) -> uint8_t { return val * factor; });

    out.push_back(color);
  }
}

template<class Pixel>
static inline void
palette_apply(const cv::Mat& src, JSOutputArray dst, Pixel palette[256]) {
  cv::Mat result(src.size(), CV_8UC3);

  printf("result.size() = %ux%u\n", result.cols, result.rows);
  printf("result.channels() = %u\n", result.channels());

  printf("src.elemSize() = %zu\n", src.elemSize());
  printf("result.elemSize() = %zu\n", result.elemSize());
  printf("result.ptr<Pixel>(0,1) - result.ptr<Pixel>(0,0) = %zu\n",
         reinterpret_cast<uchar*>(result.ptr<Pixel>(0, 1)) - reinterpret_cast<uchar*>(result.ptr<Pixel>(0, 0)));

  for(int y = 0; y < src.rows; y++) {
    for(int x = 0; x < src.cols; x++) {
      uchar index = src.at<uchar>(y, x);
      result.at<Pixel>(y, x) = palette[index];
    }
  }

  result.copyTo(dst.getMatRef());
}

static inline float
square(float x) {
  return x * x;
}

template<class ColorType>
static inline JSColorData<float>
color3f(const ColorType& c) {
  JSColorData<float> ret;
  ret.b = float(c.b) / 255.0;
  ret.g = float(c.g) / 255.0;
  ret.r = float(c.r) / 255.0;
  return ret;
}

template<class ColorType>
static inline float
color_distance_squared(const ColorType& c1, const ColorType& c2) {
  JSColorData<float> a = color3f(c1), b = color3f(c2);

  // NOTE: from https://www.compuphase.com/cmetric.htm
  float mean_r = (a.r + b.r) * 0.5f;
  float dr = a.r - b.r;
  float dg = a.g - b.g;
  float db = a.b - b.b;

  return ((2.0f + mean_r * (1.0f / 256.0f)) * square(dr) + (4.0f * square(dg)) +
          (2.0f + ((255.0f - mean_r) * (1.0f / 256.0f))) * square(db));
}

template<class ColorType>
static inline int
find_nearest(const ColorType& color, const std::vector<ColorType>& palette, int skip = -1) {
  int index, ret = -1, size = palette.size();
  float distance = std::numeric_limits<float>::max();

  for(index = 0; index < size; index++) {

    if(index == skip)
      continue;

    float newdist = color_distance_squared(color, palette[index]);
    if(newdist < distance) {
      distance = newdist;
      ret = index;
    }
  }
  return ret;
}

template<class ColorType>
static inline void
palette_match(const cv::Mat& src, JSOutputArray dst, const std::vector<ColorType>& palette, int transparent = -1) {
  cv::Mat result(src.rows, src.cols, CV_8U);
  //(src.size(), CV_8UC1);
  // cv::cvtColor(src, result, cv::COLOR_BGR2GRAY);

  if(transparent == -1) {
    for(int y = 0; y < src.rows; y++) {
      for(int x = 0; x < src.cols; x++) {
        ColorType color = src.at<ColorType>(y, x);
        int index = find_nearest(color, palette);
        result.at<uchar>(y, x) = index;
      }
    }

  } else {
    for(int y = 0; y < src.rows; y++) {
      for(int x = 0; x < src.cols; x++) {
        ColorType color = src.at<ColorType>(y, x);
        int index = color.a > 127 ? find_nearest(color, palette, transparent) : transparent;
        result.at<uchar>(y, x) = index;
      }
    }
  }

  dst.assign(result); //  result.copyTo(dst.getMatRef());
}

#endif /* PALETTE_HPP */

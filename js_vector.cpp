#include "js_keypoint.hpp" // Must include before js_vector.hpp to get JSConverter<cv::KeyPoint>
#include "js_vector.hpp"

extern "C" {

int
js_vector_init(JSContext* ctx, JSModuleDef* m) {
  int result = 0;

  result = JSVector<cv::DMatch>::init(ctx, m, "DMatchVector");
  JSVector<cv::DMatch>::set_export(ctx, m, "DMatchVector");

  if(result == 0) {
    result = JSVector<cv::KeyPoint>::init(ctx, m, "KeyPointVector");
    JSVector<cv::KeyPoint>::set_export(ctx, m, "KeyPointVector");
  }
  if(result == 0) {
    result = JSVector<cv::Mat>::init(ctx, m, "MatVector");
    JSVector<cv::Mat>::set_export(ctx, m, "MatVector");
  }
  if(result == 0) {
    result = JSVector<cv::Point>::init(ctx, m, "PointVector");
    JSVector<cv::Point>::set_export(ctx, m, "PointVector");
  }
  if(result == 0) {
    result = JSVector<std::vector<cv::Point>>::init(ctx, m, "PointVectorVector");
    JSVector<std::vector<cv::Point>>::set_export(ctx, m, "PointVectorVector");
  }
  if(result == 0) {
    result = JSVector<cv::Rect>::init(ctx, m, "RectVector");
    JSVector<cv::Rect>::set_export(ctx, m, "RectVector");
  }
  if(result == 0) {
    result = JSVector<int>::init(ctx, m, "IntVector");
    JSVector<int>::set_export(ctx, m, "IntVector");
  }
  if(result == 0) {
    result = JSVector<float>::init(ctx, m, "FloatVector");
    JSVector<float>::set_export(ctx, m, "FloatVector");
  }
  if(result == 0) {
    result = JSVector<double>::init(ctx, m, "DoubleVector");
    JSVector<double>::set_export(ctx, m, "DoubleVector");
  }
  if(result == 0) {
    result = JSVector<char>::init(ctx, m, "CharVector");
    JSVector<char>::set_export(ctx, m, "CharVector");
  }
  if(result == 0) {
    result = JSVector<std::string>::init(ctx, m, "StringVector");
    JSVector<std::string>::set_export(ctx, m, "StringVector");
  }

  return result;
}

void
js_vector_export(JSContext* ctx, JSModuleDef* m) {
  JSVector<cv::DMatch>::add_export(ctx, m, "DMatchVector");
  JSVector<cv::KeyPoint>::add_export(ctx, m, "KeyPointVector");
  JSVector<cv::Mat>::add_export(ctx, m, "MatVector");
  JSVector<cv::Point>::add_export(ctx, m, "PointVector");
  JSVector<std::vector<cv::Point>>::add_export(ctx, m, "PointVectorVector");
  JSVector<cv::Rect>::add_export(ctx, m, "RectVector");
  JSVector<int>::add_export(ctx, m, "IntVector");
  JSVector<float>::add_export(ctx, m, "FloatVector");
  JSVector<double>::add_export(ctx, m, "DoubleVector");
  JSVector<char>::add_export(ctx, m, "CharVector");
  JSVector<std::string>::add_export(ctx, m, "StringVector");
}
}

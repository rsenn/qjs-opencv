#include "include/jsbindings.hpp"
#include "js_keypoint.hpp" // Must include before js_vector.hpp to get JSConverter<cv::KeyPoint>
#include "js_vector.hpp"

extern "C" {

int
js_vector_init(JSContext* ctx, JSModuleDef* m) {
  int result = 0;

  if(result == 0) {
    result = JSVector<cv::DMatch>::init(ctx, m, "DMatchVector");
    JSVector<cv::DMatch>::set_export(ctx, m, "DMatchVector");
  }
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
    result = JSVector<cv::Point2f>::init(ctx, m, "Point2fVector");
    JSVector<cv::Point2f>::set_export(ctx, m, "Point2fVector");
  }
  if(result == 0) {
    result = JSVector<cv::Point3f>::init(ctx, m, "Point3fVector");
    JSVector<cv::Point3f>::set_export(ctx, m, "Point3fVector");
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

  if(result == 0) {
    result = JSVector<std::vector<cv::DMatch>>::init(ctx, m, "DMatchVectorVector");
    JSVector<std::vector<cv::DMatch>>::set_export(ctx, m, "DMatchVectorVector");
  }
  if(result == 0) {
    result = JSVector<std::vector<cv::KeyPoint>>::init(ctx, m, "KeyPointVectorVector");
    JSVector<std::vector<cv::KeyPoint>>::set_export(ctx, m, "KeyPointVectorVector");
  }
  if(result == 0) {
    result = JSVector<std::vector<cv::Point>>::init(ctx, m, "PointVectorVector");
    JSVector<std::vector<cv::Point>>::set_export(ctx, m, "PointVectorVector");
  }
  if(result == 0) {
    result = JSVector<std::vector<char>>::init(ctx, m, "CharVectorVector");
    JSVector<std::vector<char>>::set_export(ctx, m, "CharVectorVector");
  }

  return result;
}

void
js_vector_export(JSContext* ctx, JSModuleDef* m) {
  JSVector<cv::DMatch>::add_export(ctx, m, "DMatchVector");
  JSVector<cv::KeyPoint>::add_export(ctx, m, "KeyPointVector");
  JSVector<cv::Mat>::add_export(ctx, m, "MatVector");
  JSVector<cv::Point>::add_export(ctx, m, "PointVector");
  JSVector<cv::Point2f>::add_export(ctx, m, "Point2fVector");
  JSVector<cv::Point3f>::add_export(ctx, m, "Point3fVector");
  JSVector<cv::Rect>::add_export(ctx, m, "RectVector");
  JSVector<int>::add_export(ctx, m, "IntVector");
  JSVector<float>::add_export(ctx, m, "FloatVector");
  JSVector<double>::add_export(ctx, m, "DoubleVector");
  JSVector<char>::add_export(ctx, m, "CharVector");
  JSVector<std::string>::add_export(ctx, m, "StringVector");

  JSVector<std::vector<cv::DMatch>>::add_export(ctx, m, "DMatchVectorVector");
  JSVector<std::vector<cv::KeyPoint>>::add_export(ctx, m, "KeyPointVectorVector");
  JSVector<std::vector<cv::Point>>::add_export(ctx, m, "PointVectorVector");
  JSVector<std::vector<char>>::add_export(ctx, m, "CharVectorVector");
}
}

JSInputArray
js_vector_inputarray(JSValueConst value) {
  if(auto* ptr = JSVector<cv::Mat>::fromJS(value))
    return *ptr;
  if(auto* ptr = JSVector<cv::Point>::fromJS(value))
    return *ptr;
  if(auto* ptr = JSVector<cv::Point2f>::fromJS(value))
    return *ptr;
  if(auto* ptr = JSVector<cv::Point3f>::fromJS(value))
    return *ptr;
  if(auto* ptr = JSVector<cv::Rect>::fromJS(value))
    return *ptr;
  if(auto* ptr = JSVector<int>::fromJS(value))
    return *ptr;
  if(auto* ptr = JSVector<float>::fromJS(value))
    return *ptr;
  if(auto* ptr = JSVector<double>::fromJS(value))
    return *ptr;
  if(auto* ptr = JSVector<char>::fromJS(value))
    return *ptr;
  /*if(auto* ptr = JSVector<std::string>::fromJS(value))
    return *ptr;
  if(auto* ptr = JSVector<std::vector<cv::DMatch>>::fromJS(value))
    return *ptr;
  if(auto* ptr = JSVector<std::vector<cv::KeyPoint>>::fromJS(value))
    return *ptr;*/
  if(auto* ptr = JSVector<std::vector<cv::Point>>::fromJS(value))
    return *ptr;
  if(auto* ptr = JSVector<std::vector<char>>::fromJS(value))
    return *ptr;

  return cv::noArray();
}

JSInputOutputArray
js_vector_inputoutputarray(JSValueConst value) {
  if(auto* ptr = JSVector<cv::Mat>::fromJS(value))
    return *ptr;
  if(auto* ptr = JSVector<cv::Point>::fromJS(value))
    return *ptr;
  if(auto* ptr = JSVector<cv::Point2f>::fromJS(value))
    return *ptr;
  if(auto* ptr = JSVector<cv::Point3f>::fromJS(value))
    return *ptr;
  if(auto* ptr = JSVector<cv::Rect>::fromJS(value))
    return *ptr;
  if(auto* ptr = JSVector<int>::fromJS(value))
    return *ptr;
  if(auto* ptr = JSVector<float>::fromJS(value))
    return *ptr;
  if(auto* ptr = JSVector<double>::fromJS(value))
    return *ptr;
  if(auto* ptr = JSVector<char>::fromJS(value))
    return *ptr;
  /*if(auto* ptr = JSVector<std::string>::fromJS(value))
    return *ptr;
  if(auto* ptr = JSVector<std::vector<cv::DMatch>>::fromJS(value))
    return *ptr;
  if(auto* ptr = JSVector<std::vector<cv::KeyPoint>>::fromJS(value))
    return *ptr;*/
  if(auto* ptr = JSVector<std::vector<cv::Point>>::fromJS(value))
    return *ptr;
  if(auto* ptr = JSVector<std::vector<char>>::fromJS(value))
    return *ptr;

  return cv::noArray();
}

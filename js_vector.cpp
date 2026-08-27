#include "include/jsbindings.hpp"
#include "js_keypoint.hpp" // Must include before js_vector.hpp to get JSConverter<cv::KeyPoint>
#include "js_vector.hpp"

using cv::Point;
using cv::Point2f;
using std::vector;

template<class T>
static inline void
init_vector(JSContext* ctx, JSModuleDef* m, const char* name, int& result) {
  if(result == 0) {
    result = JSVector<T>::init(ctx, m, name);
    JSVector<T>::set_export(ctx, m, name);
  }
}

extern "C" {

int
js_vector_init(JSContext* ctx, JSModuleDef* m) {
  int result = 0;

  init_vector<cv::DMatch>(ctx, m, "DMatchVector", result);
  init_vector<cv::KeyPoint>(ctx, m, "KeyPointVector", result);
  init_vector<cv::Mat>(ctx, m, "MatVector", result);
  init_vector<cv::Point>(ctx, m, "PointVector", result);
  init_vector<cv::Point2f>(ctx, m, "Point2fVector", result);
  init_vector<cv::Point3f>(ctx, m, "Point3fVector", result);
  init_vector<cv::Rect>(ctx, m, "RectVector", result);
  init_vector<int>(ctx, m, "IntVector", result);
  init_vector<float>(ctx, m, "FloatVector", result);
  init_vector<double>(ctx, m, "DoubleVector", result);
  init_vector<char>(ctx, m, "CharVector", result);
  /*init_vector<std::string>(ctx, m, "StringVector", result);

  init_vector<vector<cv::DMatch>>(ctx, m, "DMatchVectorVector", result);
  init_vector<vector<cv::KeyPoint>>(ctx, m, "KeyPointVectorVector", result);*/
  init_vector<vector<Point>>(ctx, m, "PointVectorVector", result);
  init_vector<vector<Point2f>>(ctx, m, "Point2fVectorVector", result);
  init_vector<vector<char>>(ctx, m, "CharVectorVector", result);

  return result;
}

void
js_vector_export(JSContext* ctx, JSModuleDef* m) {
  JSVector<cv::DMatch>::add_export(ctx, m, "DMatchVector");
  JSVector<cv::KeyPoint>::add_export(ctx, m, "KeyPointVector");
  JSVector<cv::Mat>::add_export(ctx, m, "MatVector");
  JSVector<Point>::add_export(ctx, m, "PointVector");
  JSVector<Point2f>::add_export(ctx, m, "Point2fVector");
  JSVector<cv::Point3f>::add_export(ctx, m, "Point3fVector");
  JSVector<cv::Rect>::add_export(ctx, m, "RectVector");
  JSVector<int>::add_export(ctx, m, "IntVector");
  JSVector<float>::add_export(ctx, m, "FloatVector");
  JSVector<double>::add_export(ctx, m, "DoubleVector");
  JSVector<char>::add_export(ctx, m, "CharVector");
  /*JSVector<std::string>::add_export(ctx, m, "StringVector");

  JSVector<vector<cv::DMatch>>::add_export(ctx, m, "DMatchVectorVector");
  JSVector<vector<cv::KeyPoint>>::add_export(ctx, m, "KeyPointVectorVector");*/
  JSVector<vector<Point>>::add_export(ctx, m, "PointVectorVector");
  JSVector<vector<Point2f>>::add_export(ctx, m, "Point2fVectorVector");
  JSVector<vector<char>>::add_export(ctx, m, "CharVectorVector");
}
}

JSInputArray
js_vector_inputarray(JSValueConst value) {
  if(auto* ptr = JSVector<cv::Mat>::fromJS(value))
    return *ptr;
  if(auto* ptr = JSVector<Point>::fromJS(value))
    return *ptr;
  if(auto* ptr = JSVector<Point2f>::fromJS(value))
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
  if(auto* ptr = JSVector<vector<cv::DMatch>>::fromJS(value))
    return *ptr;
  if(auto* ptr = JSVector<vector<cv::KeyPoint>>::fromJS(value))
    return *ptr;*/
  if(auto* ptr = JSVector<vector<Point>>::fromJS(value))
    return *ptr;
  if(auto* ptr = JSVector<vector<Point2f>>::fromJS(value))
    return *ptr;
  if(auto* ptr = JSVector<vector<char>>::fromJS(value))
    return *ptr;

  return cv::noArray();
}

JSInputOutputArray
js_vector_inputoutputarray(JSValueConst value) {
  if(auto* ptr = JSVector<cv::Mat>::fromJS(value))
    return *ptr;
  if(auto* ptr = JSVector<Point>::fromJS(value))
    return *ptr;
  if(auto* ptr = JSVector<Point2f>::fromJS(value))
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
  if(auto* ptr = JSVector<vector<cv::DMatch>>::fromJS(value))
    return *ptr;
  if(auto* ptr = JSVector<vector<cv::KeyPoint>>::fromJS(value))
    return *ptr;*/
  if(auto* ptr = JSVector<vector<Point>>::fromJS(value))
    return *ptr;
  if(auto* ptr = JSVector<vector<Point2f>>::fromJS(value))
    return *ptr;
  if(auto* ptr = JSVector<vector<char>>::fromJS(value))
    return *ptr;

  return cv::noArray();
}

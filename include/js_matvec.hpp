#ifndef JS_MATVEC_HPP
#define JS_MATVEC_HPP

#include "include/js_inputoutputarray.hpp"
#include "js_mat.hpp"

#include <opencv2/core.hpp>

#include <vector>

/**
 * @brief A cv::InputArrayOfArrays / OutputArrayOfArrays argument backed by a
 * JS array of Mat (what opencv.js calls a MatVector, see BUGS
 * no-opencvjs-matvector). calib3d takes lists of per-view point Mats
 * (objectPoints, imagePoints) and returns lists (rvecs, tvecs); the plain
 * js_cv_inputarray()/js_cv_outputarray() resolvers cannot express those.
 *
 * Elements that are not a Mat are read as point lists ([x, y(, z)] arrays or
 * {x, y(, z)} objects) into an Nx1 CV_32FC2/3 Mat. A value that is no JS
 * array falls back to the plain resolvers (a single Mat, a typed array).
 * With rows=true an Nx1 element is reshaped (no copy) to 1xN: OpenCV 5's
 * calibration functions only take per-view point lists as one row (4.x took
 * either), and a Mat from projectPoints()/findChessboardCorners() is Nx1.
 * The vector is written back into the JS array (cleared first) when the
 * object goes out of scope, so a cv::Exception still leaves the array in a
 * consistent state.
 */
class JSMatVec {
public:
  JSContext* ctx;
  JSValue value;
  bool is_array;
  bool writeback;
  bool rows;
  std::vector<cv::Mat> vec;

  JSMatVec(JSContext* c, JSValueConst v, bool out = false, bool row = false)
      : ctx(c), value(JS_DupValue(c, v)), is_array(JS_IsArray(c, v)), writeback(out), rows(row) {
    if(is_array)
      read(); /* also for outputs: the caller may pass initial contents */
  }

  ~JSMatVec() {
    if(is_array && writeback) {
      JS_SetPropertyStr(ctx, value, "length", JS_NewInt32(ctx, 0));

      for(size_t i = 0; i < vec.size(); i++)
        JS_SetPropertyUint32(ctx, value, i, js_mat_wrap(ctx, vec[i]));
    }

    JS_FreeValue(ctx, value);
  }

  JSMatVec(const JSMatVec&) = delete;
  JSMatVec& operator=(const JSMatVec&) = delete;

  cv::_InputArray
  in() {
    return is_array ? cv::_InputArray(vec) : js_cv_inputarray(ctx, value);
  }

  cv::_OutputArray
  out() {
    return is_array ? cv::_OutputArray(vec) : js_cv_outputarray(ctx, value);
  }

  cv::_InputOutputArray
  inout() {
    return is_array ? cv::_InputOutputArray(vec) : js_cv_inputoutputarray(ctx, value);
  }

private:
  static cv::Mat
  points(JSContext* ctx, JSValueConst list) {
    std::vector<cv::Vec3d> pts;
    int cn = 2;
    int64_t len = 0;
    JSValue lv = JS_GetPropertyStr(ctx, list, "length");

    JS_ToInt64(ctx, &len, lv);
    JS_FreeValue(ctx, lv);

    for(int64_t i = 0; i < len; i++) {
      JSValue pt = JS_GetPropertyUint32(ctx, list, i);
      double v[3] = {0, 0, 0};
      int32_t n = 0;
      JSValue l = JS_GetPropertyStr(ctx, pt, "length");

      JS_ToInt32(ctx, &n, l);
      JS_FreeValue(ctx, l);

      if(n >= 2) {
        for(int k = 0; k < 3 && k < n; k++) {
          JSValue c = JS_GetPropertyUint32(ctx, pt, k);

          JS_ToFloat64(ctx, &v[k], c);
          JS_FreeValue(ctx, c);
        }
        if(n >= 3)
          cn = 3;
      } else {
        static const char* const names[] = {"x", "y", "z"};

        for(int k = 0; k < 3; k++) {
          JSValue c = JS_GetPropertyStr(ctx, pt, names[k]);

          if(!JS_IsUndefined(c)) {
            JS_ToFloat64(ctx, &v[k], c);
            if(k == 2)
              cn = 3;
          }
          JS_FreeValue(ctx, c);
        }
      }

      JS_FreeValue(ctx, pt);
      pts.push_back(cv::Vec3d(v[0], v[1], v[2]));
    }

    cv::Mat m(int(pts.size()), 1, CV_32FC(cn));

    for(size_t i = 0; i < pts.size(); i++)
      for(int k = 0; k < cn; k++)
        m.ptr<float>(int(i))[k] = float(pts[i][k]);

    return m;
  }

  void
  read() {
    int64_t len = 0;
    JSValue lv = JS_GetPropertyStr(ctx, value, "length");

    JS_ToInt64(ctx, &len, lv);
    JS_FreeValue(ctx, lv);

    for(int64_t i = 0; i < len; i++) {
      JSValue el = JS_GetPropertyUint32(ctx, value, i);
      cv::Mat* m = js_mat_data_nothrow(el);

      cv::Mat mat = m ? *m : points(ctx, el);

      if(rows && mat.dims == 2 && mat.cols == 1 && mat.rows > 1 && mat.isContinuous())
        mat = mat.reshape(mat.channels(), 1);

      vec.push_back(mat);
      JS_FreeValue(ctx, el);
    }
  }
};

#endif /* defined(JS_MATVEC_HPP) */

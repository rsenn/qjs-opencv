#ifndef TYPES_HPP
#define TYPES_HPP

#include <vector>
#include <opencv2/core.hpp>

namespace cv {
using DMatchVector = std::vector<cv::DMatch>;
using KeyPointVector = std::vector<cv::KeyPoint>;
using MatVector = std::vector<cv::Mat>;
using PointVector = std::vector<cv::Point>;
using Point2fVector = std::vector<cv::Point2f>;
using Point3fVector = std::vector<cv::Point3f>;
using RectVector = std::vector<cv::Rect>;
using IntVector = std::vector<int>;
using FloatVector = std::vector<float>;
using DoubleVector = std::vector<double>;
using CharVector = std::vector<char>;
using StringVector = std::vector<std::string>;
using DMatchVectorVector = std::vector<std::vector<cv::DMatch>>;
using KeyPointVectorVector = std::vector<std::vector<cv::KeyPoint>>;
using PointVectorVector = std::vector<std::vector<cv::Point>>;
using Point2fVectorVector = std::vector<std::vector<cv::Point2f>>;
using CharVectorVector = std::vector<std::vector<char>>;
} // namespace cv

#endif // TYPES_HPP

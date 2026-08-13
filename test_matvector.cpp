#include <opencv2/opencv.hpp>
#include <iostream>
#include <iomanip>

static const char* depth_name(int d) {
    switch(d) {
        case CV_8U:  return "CV_8U";
        case CV_8S:  return "CV_8S";
        case CV_16U: return "CV_16U";
        case CV_16S: return "CV_16S";
        case CV_32S: return "CV_32S";
        case CV_32F: return "CV_32F";
        case CV_64F: return "CV_64F";
        default:     return "UNKNOWN";
    }
}

static const char* type_name(int t) {
    switch(t) {
        case CV_8UC1:  return "CV_8UC1";
        case CV_8UC2:  return "CV_8UC2";
        case CV_8UC3:  return "CV_8UC3";
        case CV_8UC4:  return "CV_8UC4";
        case CV_8SC1:  return "CV_8SC1";
        case CV_16UC1: return "CV_16UC1";
        case CV_16SC1: return "CV_16SC1";
        case CV_32SC1: return "CV_32SC1";
        case CV_32SC2: return "CV_32SC2";
        case CV_32SC3: return "CV_32SC3";
        case CV_32FC1: return "CV_32FC1";
        case CV_32FC2: return "CV_32FC2";
        case CV_64FC1: return "CV_64FC1";
        default:       return "OTHER";
    }
}

int main() {
    // 1. Create 200x200 black image
    cv::Mat img(200, 200, CV_8UC1, cv::Scalar(0));

    // 2. Draw shapes
    // Rectangle
    cv::rectangle(img, cv::Point(10, 10), cv::Point(60, 60), cv::Scalar(255), 1);
    // Circle
    cv::circle(img, cv::Point(130, 40), 30, cv::Scalar(255), 1);
    // Triangle via polylines
    std::vector<cv::Point> tri = {
        cv::Point(100, 150), cv::Point(130, 100), cv::Point(160, 150), cv::Point(100, 150)
    };
    cv::polylines(img, tri, false, cv::Scalar(255), 1);

    // === vector<Mat> variant ===
    std::cout << "===== findContours with vector<Mat> =====\n";
    std::vector<cv::Mat> mat_contours;
    cv::Mat hierarchy_mat;
    cv::findContours(img, mat_contours, hierarchy_mat, cv::RETR_TREE, cv::CHAIN_APPROX_NONE);

    std::cout << "Number of contours: " << mat_contours.size() << "\n";
    std::cout << "Hierarchy mat: type=" << type_name(hierarchy_mat.type())
              << " size=" << hierarchy_mat.size() << " cols=" << hierarchy_mat.cols
              << " rows=" << hierarchy_mat.rows << "\n\n";

    for (size_t i = 0; i < mat_contours.size(); i++) {
        const cv::Mat& m = mat_contours[i];
        std::cout << "--- Contour Mat[" << i << "] ---\n";
        std::cout << "  cols=" << m.cols << " rows=" << m.rows << "\n";
        std::cout << "  type()=" << type_name(m.type()) << " (" << m.type() << ")\n";
        std::cout << "  depth()=" << depth_name(m.depth()) << " (" << m.depth() << ")\n";
        std::cout << "  total()=" << m.total() << "\n";
        std::cout << "  channels()=" << m.channels() << "\n";
        std::cout << "  size=" << m.size << "\n";
        std::cout << "  step[0]=" << m.step[0] << " step[1]=" << m.step[1] << "\n";
        std::cout << "  isContinuous=" << m.isContinuous() << "\n";
        std::cout << "  dims=" << m.dims << "\n";

        // Dump first few points
        int npts = (int)m.total();
        int show = std::min(npts, 8);
        if (m.type() == CV_32SC2) {
            const cv::Point* pts = (const cv::Point*)m.data;
            std::cout << "  First " << show << " points (as cv::Point*): ";
            for (int j = 0; j < show; j++) std::cout << "(" << pts[j].x << "," << pts[j].y << ") ";
            std::cout << "\n";
        } else if (m.type() == CV_32SC1) {
            const int* data = (const int*)m.data;
            std::cout << "  First " << show << " raw ints: ";
            for (int j = 0; j < show; j++) std::cout << data[j] << " ";
            std::cout << "\n";
        }
        std::cout << "\n";
    }

    // === vector<vector<Point>> variant ===
    std::cout << "\n===== findContours with vector<vector<Point>> =====\n";
    std::vector<std::vector<cv::Point>> vec_contours;
    std::vector<cv::Vec4i> hierarchy_vec;
    cv::findContours(img, vec_contours, hierarchy_vec, cv::RETR_TREE, cv::CHAIN_APPROX_NONE);

    std::cout << "Number of contours: " << vec_contours.size() << "\n\n";
    for (size_t i = 0; i < vec_contours.size(); i++) {
        std::cout << "--- Contour[" << i << "] ---\n";
        std::cout << "  size=" << vec_contours[i].size() << "\n";
        int show = std::min((int)vec_contours[i].size(), 8);
        std::cout << "  First " << show << " points: ";
        for (int j = 0; j < show; j++) {
            std::cout << "(" << vec_contours[i][j].x << "," << vec_contours[i][j].y << ") ";
        }
        std::cout << "\n\n";
    }

    // === Comparison ===
    std::cout << "\n===== Comparison =====\n";
    std::cout << "mat_contours.size() == vec_contours.size(): "
              << (mat_contours.size() == vec_contours.size() ? "YES" : "NO") << "\n";
    for (size_t i = 0; i < std::min(mat_contours.size(), vec_contours.size()); i++) {
        int mat_pts = (int)mat_contours[i].total();
        int vec_pts = (int)vec_contours[i].size();
        std::cout << "  Contour[" << i << "]: mat.total()=" << mat_pts
                  << " vs vec.size()=" << vec_pts
                  << (mat_pts == vec_pts ? " (MATCH)" : " (DIFFER)") << "\n";

        // Check point data matches
        if (mat_pts == vec_pts && mat_contours[i].type() == CV_32SC2) {
            const cv::Point* mpts = (const cv::Point*)mat_contours[i].data;
            bool match = true;
            for (int j = 0; j < mat_pts; j++) {
                if (mpts[j] != vec_contours[i][j]) { match = false; break; }
            }
            std::cout << "    Point data: " << (match ? "IDENTICAL" : "DIFFERENT") << "\n";
        }
    }

    return 0;
}

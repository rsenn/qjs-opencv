#include <opencv2/opencv.hpp>
#include <iostream>

using namespace cv;
using namespace std;

int main() {
    Mat image = Mat::zeros(200, 200, CV_8UC1);
    rectangle(image, Rect(50, 50, 100, 100), Scalar(255), -1);
    circle(image, Point(150, 150), 30, Scalar(255), -1);

    cout << "Testing PointVector/Vec4i as Contours container:\n\n";

    // Test 1: vector<Point> as flat container for all contours
    {
        cout << "1. vector<Point> (PointVector) as flat container:\n";
        try {
            vector<Point> all_points;
            vector<Vec4i> hierarchy;
            findContours(image, all_points, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
            cout << "   SUCCESS: " << all_points.size() << " points total\n";
        } catch (const cv::Exception& e) {
            cout << "   FAILED: " << e.what() << "\n";
        }
        cout << "\n";
    }

    // Test 2: vector<Vec4i> as flat container
    {
        cout << "2. vector<Vec4i> as flat container:\n";
        try {
            vector<Vec4i> all_vec4i;
            vector<Vec4i> hierarchy;
            findContours(image, all_vec4i, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
            cout << "   SUCCESS: " << all_vec4i.size() << " Vec4i total\n";
        } catch (const cv::Exception& e) {
            cout << "   FAILED: " << e.what() << "\n";
        }
        cout << "\n";
    }

    // Test 3: vector<Mat> as reference (known to work)
    {
        cout << "3. vector<Mat> (MatVector) as reference:\n";
        vector<Mat> contours;
        vector<Vec4i> hierarchy;
        findContours(image, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
        cout << "   SUCCESS: " << contours.size() << " contours\n";
        size_t total_points = 0;
        for (const auto& c : contours) {
            total_points += c.rows;
        }
        cout << "   Total points: " << total_points << "\n";
        cout << "\n";
    }

    // Test 4: Check if HoughLinesP uses PointVector differently
    {
        cout << "4. HoughLinesP with vector<Vec4i> (for comparison):\n";
        Mat edges;
        Canny(image, edges, 50, 150);
        try {
            vector<Vec4i> lines;
            HoughLinesP(edges, lines, 1, CV_PI/180, 50, 50, 10);
            cout << "   SUCCESS: " << lines.size() << " lines detected\n";
            cout << "   Note: HoughLinesP DOES accept vector<Vec4i>\n";
        } catch (const cv::Exception& e) {
            cout << "   FAILED: " << e.what() << "\n";
        }
        cout << "\n";
    }

    cout << "Conclusion:\n";
    cout << "- findContours requires OutputArrayOfArrays (nested structure)\n";
    cout << "- HoughLinesP accepts OutputArray (flat structure)\n";
    cout << "- PointVector works for HoughLinesP but not findContours\n";
    cout << "- MatVector is the only viable option for Contours\n";

    return 0;
}

#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>

using namespace cv;
using namespace std;

int main() {
    // Create a test image with a simple shape
    Mat image = Mat::zeros(200, 200, CV_8UC1);
    rectangle(image, Rect(50, 50, 100, 100), Scalar(255), -1);
    
    cout << "Testing findContours with different output types:\n\n";
    
    // Test 1: vector<vector<Point>> - standard approach
    {
        cout << "1. vector<vector<Point>> (standard):\n";
        vector<vector<Point>> contours;
        vector<Vec4i> hierarchy;
        findContours(image, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
        cout << "   Contours found: " << contours.size() << "\n";
        if (!contours.empty()) {
            cout << "   First contour points: " << contours[0].size() << "\n";
            cout << "   Point type size: " << sizeof(Point) << " bytes\n";
            cout << "   First 3 points: ";
            for (int i = 0; i < min(3, (int)contours[0].size()); i++) {
                cout << "(" << contours[0][i].x << "," << contours[0][i].y << ") ";
            }
            cout << "\n";
        }
        cout << "   Hierarchy size: " << hierarchy.size() << "\n\n";
    }
    
    // Test 2: vector<Mat> - already tested
    {
        cout << "2. vector<Mat>:\n";
        vector<Mat> contours;
        Mat hierarchy;
        findContours(image, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
        cout << "   Contours found: " << contours.size() << "\n";
        if (!contours.empty()) {
            cout << "   First contour type: " << contours[0].type() << " (CV_32SC2=" << CV_32SC2 << ")\n";
            cout << "   First contour size: " << contours[0].rows << "x" << contours[0].cols << "\n";
        }
        cout << "   Hierarchy type: " << hierarchy.type() << " (CV_32SC4=" << CV_32SC4 << ")\n\n";
    }
    
    // Test 3: vector<Point> - can this work?
    {
        cout << "3. vector<Point> (PointVector - single flat vector):\n";
        try {
            vector<Point> points;
            Mat hierarchy;
            findContours(image, points, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
            cout << "   Points found: " << points.size() << "\n";
            if (!points.empty()) {
                cout << "   First 3 points: ";
                for (int i = 0; i < min(3, (int)points.size()); i++) {
                    cout << "(" << points[i].x << "," << points[i].y << ") ";
                }
                cout << "\n";
            }
            cout << "   WARNING: This flattened all contours into one vector!\n";
        } catch (const cv::Exception& e) {
            cout << "   Exception: " << e.what() << "\n";
        }
        cout << "\n";
    }
    
    // Test 4: vector<Vec4i> - can this work for points?
    {
        cout << "4. vector<Vec4i>:\n";
        try {
            vector<Vec4i> vec4i_contours;
            vector<Vec4i> hierarchy;
            findContours(image, vec4i_contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
            cout << "   Contours found: " << vec4i_contours.size() << "\n";
            if (!vec4i_contours.empty()) {
                cout << "   First Vec4i: [" 
                     << vec4i_contours[0][0] << "," 
                     << vec4i_contours[0][1] << "," 
                     << vec4i_contours[0][2] << "," 
                     << vec4i_contours[0][3] << "]\n";
                cout << "   Vec4i type size: " << sizeof(Vec4i) << " bytes\n";
                cout << "   Point type size: " << sizeof(Point) << " bytes\n";
                cout << "   Note: Vec4i has 4 ints, Point has 2 ints\n";
                cout << "   WARNING: This flattened all contours - each Vec4i is 2 points!\n";
            }
        } catch (const cv::Exception& e) {
            cout << "   Exception: " << e.what() << "\n";
        }
        cout << "\n";
    }
    
    // Test 5: vector<vector<Vec4i>> - alternative nested structure
    {
        cout << "5. vector<vector<Vec4i>>:\n";
        try {
            vector<vector<Vec4i>> contours;
            vector<Vec4i> hierarchy;
            findContours(image, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
            cout << "   Contours found: " << contours.size() << "\n";
            if (!contours.empty()) {
                cout << "   First contour Vec4i count: " << contours[0].size() << "\n";
                cout << "   First Vec4i: [" 
                     << contours[0][0][0] << "," 
                     << contours[0][0][1] << "," 
                     << contours[0][0][2] << "," 
                     << contours[0][0][3] << "]\n";
            }
        } catch (const cv::Exception& e) {
            cout << "   Exception: " << e.what() << "\n";
        }
        cout << "\n";
    }
    
    // Test 6: Check if Point is actually Vec2i
    {
        cout << "6. Type size comparison:\n";
        cout << "   sizeof(Point): " << sizeof(Point) << " bytes\n";
        cout << "   sizeof(Vec2i): " << sizeof(Vec2i) << " bytes\n";
        cout << "   sizeof(Vec4i): " << sizeof(Vec4i) << " bytes\n";
        cout << "   Point is typedef of Vec2i: " << (sizeof(Point) == sizeof(Vec2i) ? "YES" : "NO") << "\n";
    }
    
    return 0;
}

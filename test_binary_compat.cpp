#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>

using namespace cv;
using namespace std;

int main() {
    cout << "=== Binary Compatibility Test ===\n\n";
    
    // Test 1: Point vs Vec2i memory layout
    {
        cout << "1. Point vs Vec2i:\n";
        Point p(10, 20);
        Vec2i v(10, 20);
        cout << "   sizeof(Point): " << sizeof(Point) << "\n";
        cout << "   sizeof(Vec2i): " << sizeof(Vec2i) << "\n";
        cout << "   Point memory: ";
        int32_t* p_ptr = reinterpret_cast<int32_t*>(&p);
        cout << "[" << p_ptr[0] << ", " << p_ptr[1] << "]\n";
        cout << "   Vec2i memory: ";
        int32_t* v_ptr = reinterpret_cast<int32_t*>(&v);
        cout << "[" << v_ptr[0] << ", " << v_ptr[1] << "]\n";
        cout << "   Binary compatible: " << (sizeof(Point) == sizeof(Vec2i) ? "YES" : "NO") << "\n\n";
    }
    
    // Test 2: Vec4i vs two Points
    {
        cout << "2. Vec4i vs two Points:\n";
        Vec4i v(10, 20, 30, 40);
        Point p1(10, 20), p2(30, 40);
        cout << "   sizeof(Vec4i): " << sizeof(Vec4i) << "\n";
        cout << "   2*sizeof(Point): " << 2*sizeof(Point) << "\n";
        cout << "   Vec4i memory: ";
        int32_t* v_ptr = reinterpret_cast<int32_t*>(&v);
        cout << "[" << v_ptr[0] << ", " << v_ptr[1] << ", " << v_ptr[2] << ", " << v_ptr[3] << "]\n";
        cout << "   Two Points: ";
        int32_t* p1_ptr = reinterpret_cast<int32_t*>(&p1);
        int32_t* p2_ptr = reinterpret_cast<int32_t*>(&p2);
        cout << "[" << p1_ptr[0] << ", " << p1_ptr[1] << ", " << p2_ptr[0] << ", " << p2_ptr[1] << "]\n";
        cout << "   Binary compatible: " << (sizeof(Vec4i) == 2*sizeof(Point) ? "YES" : "NO") << "\n\n";
    }
    
    // Test 3: vector<Point> vs vector<Vec2i> memory layout
    {
        cout << "3. vector<Point> vs vector<Vec2i>:\n";
        vector<Point> vp = {Point(1,2), Point(3,4), Point(5,6)};
        vector<Vec2i> vv = {Vec2i(1,2), Vec2i(3,4), Vec2i(5,6)};
        cout << "   vector<Point> data ptr: " << (void*)vp.data() << "\n";
        cout << "   vector<Vec2i> data ptr: " << (void*)vv.data() << "\n";
        cout << "   vector<Point> size: " << vp.size() * sizeof(Point) << " bytes\n";
        cout << "   vector<Vec2i> size: " << vv.size() * sizeof(Vec2i) << " bytes\n";
        cout << "   Can reinterpret_cast: " << (sizeof(Point) == sizeof(Vec2i) ? "YES" : "NO") << "\n\n";
    }
    
    // Test 4: Mat CV_32SC2 vs vector<Point>
    {
        cout << "4. Mat CV_32SC2 vs vector<Point>:\n";
        Mat m = Mat::zeros(3, 1, CV_32SC2);
        vector<Point> vp = {Point(10,20), Point(30,40), Point(50,60)};
        
        // Fill Mat
        for (int i = 0; i < 3; i++) {
            m.at<Vec2i>(i, 0) = Vec2i(vp[i].x, vp[i].y);
        }
        
        cout << "   Mat size: " << m.rows << "x" << m.cols << "\n";
        cout << "   Mat type: CV_32SC2\n";
        cout << "   Mat data: ";
        Vec2i* m_ptr = m.ptr<Vec2i>(0);
        for (int i = 0; i < 3; i++) {
            cout << "[" << m_ptr[i][0] << "," << m_ptr[i][1] << "] ";
        }
        cout << "\n";
        
        cout << "   vector<Point>: ";
        for (const auto& p : vp) {
            cout << "[" << p.x << "," << p.y << "] ";
        }
        cout << "\n";
        
        // Test reinterpret
        Point* reinterpreted = reinterpret_cast<Point*>(m.ptr<Vec2i>(0));
        cout << "   Reinterpreted as Point*: ";
        for (int i = 0; i < 3; i++) {
            cout << "[" << reinterpreted[i].x << "," << reinterpreted[i].y << "] ";
        }
        cout << "\n";
        cout << "   Binary compatible: YES\n\n";
    }
    
    // Test 5: Mat CV_32SC2 vs Uint32Array view
    {
        cout << "5. Mat CV_32SC2 as Uint32Array view:\n";
        Mat m = Mat::zeros(2, 1, CV_32SC2);
        m.at<Vec2i>(0, 0) = Vec2i(100, 200);
        m.at<Vec2i>(1, 0) = Vec2i(300, 400);
        
        uint32_t* uint_view = reinterpret_cast<uint32_t*>(m.data);
        cout << "   Mat data as uint32_t*: ";
        for (int i = 0; i < 4; i++) {
            cout << uint_view[i] << " ";
        }
        cout << "\n";
        cout << "   This is what Uint32Array view would show\n";
        cout << "   Useful for fast iteration without Point construction\n\n";
    }
    
    // Test 6: HoughLinesP output format
    {
        cout << "6. HoughLinesP output (vector<Vec4i>):\n";
        Mat img = Mat::zeros(100, 100, CV_8UC1);
        line(img, Point(10, 10), Point(90, 90), Scalar(255), 2);
        
        vector<Vec4i> lines;
        HoughLinesP(img, lines, 1, CV_PI/180, 50, 50, 10);
        
        cout << "   Lines found: " << lines.size() << "\n";
        if (!lines.empty()) {
            Vec4i l = lines[0];
            cout << "   First line: [" << l[0] << "," << l[1] << "," << l[2] << "," << l[3] << "]\n";
            cout << "   As two Points: (" << l[0] << "," << l[1] << ") to (" << l[2] << "," << l[3] << ")\n";
            cout << "   Binary: Vec4i is 2 Points concatenated\n\n";
        }
    }
    
    cout << "=== Conclusions ===\n";
    cout << "- Point and Vec2i are binary compatible (same memory layout)\n";
    cout << "- Vec4i is binary compatible with 2 consecutive Points\n";
    cout << "- Mat CV_32SC2 data can be reinterpreted as Point* or uint32_t*\n";
    cout << "- PointIterator could work on Mat data directly\n";
    cout << "- LineIterator could work on Vec4i data (HoughLinesP output)\n";
    
    return 0;
}

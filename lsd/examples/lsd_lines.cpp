#include <iostream>
#include <string>

#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

#include "lsd_opencv.hpp"

using namespace std;
using namespace cv;

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cout << "lsd_lines [input image]" << std::endl;
        return false;
    }

    std::string in = argv[1];

    Mat image = imread(in,cv::IMREAD_GRAYSCALE);

    // Create and LSD detector with std refinement.
    Ptr<LSD> lsd_std = createLSDPtr(::LSD_REFINE_STD);
    double start = double(getTickCount());
    vector<Vec4i> lines_std;
    lsd_std->detect(image, lines_std);
    double duration_ms = (double(getTickCount()) - start) * 1000 / getTickFrequency();
    std::cout << "OpenCV STD (blue) - " << duration_ms << " ms." << std::endl;

    // Create an LSD detector with no refinement applied.
    Ptr<LSD> lsd_none = createLSDPtr(::LSD_REFINE_NONE);
    start = double(getTickCount());
    vector<Vec4i> lines_none;
    lsd_none->detect(image, lines_none);
    duration_ms = (double(getTickCount()) - start) * 1000 / getTickFrequency();
    std::cout << "OpenCV NONE (red)- " << duration_ms << " ms." << std::endl;
    std::cout << "Overlapping pixels are shown in purple." << std::endl;

    Mat difference = Mat::zeros(image.size(), CV_8UC1);
    lsd_none->compareSegments(image.size(), lines_std, lines_none, difference);
    imshow("Line difference", difference);

    Mat drawnLines(image);
    lsd_none->drawSegments(drawnLines, lines_std);
    imshow("Standard refinement", drawnLines);

    waitKey();
    return 0;
}

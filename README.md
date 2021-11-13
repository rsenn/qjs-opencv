# qjs-opencv  

OpenCV bindings for QuickJS (https://bellard.org/quickjs/)


Build system
* CMake build system
* OpenCV from 4.2.0 on, with NONFREE modules from quickjs_contrib for the LineSegmentDetector
* Builds against vanilla QuickJS from quickjs-2021-03-27.tar.xz with -DQUICKJS_PREFIX=<directory>

There is a fork, a patched QuickJS at https://github.com/rsenn/quickjs
* adds a compile-time module search paths QUICKJS_C_MODULE_DIR and QUICKJS_JS_MODULE_DIR
* can be overriden with the environment variable QUICKJS_MODULES for searching the opencv.so module.

Usage

  import * as cv from 'opencv.so'

  let viewport = new cv.Size(640, 480);
  let mat = new cv.Mat(viewport, cv.CV_8UC3);
  let cap = new cv.VideoCapture(0);

  cap.read(mat);

  cv.imshow("qjs-opencv", mat);


Classes under namespace cv.*
* CLAHE
* Contour - Iterable container based on a std::vector<cv::Point3d>
* Draw
* FastLineDetector
* Feature2D
* KeyPoint
* Line
* LineSegmentDetector - 
* Mat - Object based on cv::Mat, calling mat(cv.Rect) makes a cv.Mat into an ROI
* MatIterator - Iterator for cv.Mat yielding TypedArray with offset 
* Point - Object based on cv::Point_<double>
* PointIterator - Iterator for ArrayBuffer/cv.Contour yielding a cv::Point_<double>
* Rect - Object based on cv::Rect_<double>
* RotatedRect - Object based on cv::RotatedRect3d
* Size - Object based on cv::Size_<double>
* SliceIterator - Iterator that partitions iterable into tuples
* Subdiv2D - Object based on cv::Size
* UMat
* TickMeter
* VideoCapture
* VideoWriter

Draw functions
* drawCircle
* drawEllipse
* drawContour
* drawLine
* drawPolygon
* drawRect
* drawKeypoints
* putText
* getTextSize
* getFontScaleFromHeight
* loadFont
* clipLine

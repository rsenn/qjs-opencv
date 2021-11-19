# qjs-opencv  

OpenCV bindings for QuickJS (https://bellard.org/quickjs/)

## Rationale
This put in use with "plot-cv" (https://github.com/rsenn/plot-cv) which converts contours from any video source to SVG ready to pass into LaserWeb4.
With this and the QuickJS interactive REPL from Fabrice Bellard you have a modern ES2020 live-coding environment for an OpenCV processing pipeline.
<<<<<<< HEAD
QuickJS is 6 files of highly portable C99 and cleverly I/O multi-plexed and in asynchronous ways interoperable with other library bindings.
=======
QuickJS is 6 files of highly portable C99 and has a lean startup time, compiling directly to byte-code without AST.
>>>>>>> 65f941b6be427846cb34f08d5f10937b795573cd

## Basic ideas implemented
* Running cv.Canny, cv.findContours algorithm, then have a cv.Contour based on an ArrayBuffer of the underlying std::vector
* Have most cv.* free-functions with abstract cv::InputOutputArray translated to the underlying (Shared-)ArrayBuffer of a cv.Mat, cv.Contour or a TypedArray.
* Go low in memory. Everything is mutable, no copies, trying to have correct finalizers.
* Much is treated as a block, a pointer into a byte-array and a size, using JS_GetArrayBuffer
* Iterators can yield TypedArrays or DataViews, having an offset and a length
* cv.Mat is optimized for Float64Array(4) iteration
* cv.Contour is optimized to iterate yielding cv.Point
* arguments can be cv.Mat and cv.Contour interchangably by cv::InputOutputArray abstraction
* have some minimal ES6 classes wrapping the HighGUI functions
  - Window, TextStyle, DrawText (using freetype renderer, not internal bitmap font)
  - Pipeline (new Pipeline([ <arrow-functions...> ]) callable lambda processing queue
  - ÏmageSequence, VideoSource that abstract cv::imread / cv::imwrite of the imgcodecs module
* additional algorithms, not in OpenCV but interoperable on cv.Mat
  - has some lo-fying of PNG and GIF images and additional palette reduction


## Build system
* Built already on x86_64 linux, aarch64 arm, x86_64 w64 mingw32, emscripten/clang wasm32 via -DCMAKE_TOOLCHAIN_FILE setup or natively
* OpenCV from 4.2.0 on, with NONFREE modules from quickjs_contrib for the LineSegmentDetector
* Builds against vanilla QuickJS from quickjs-2021-03-27.tar.xz with -DQUICKJS_PREFIX=<directory>
* or against installable QuickJS fork with module search path at https://github.com/rsenn/quickjs
  - adds a compile-time module search paths QUICKJS_C_MODULE_DIR and QUICKJS_JS_MODULE_DIR
  - can be overriden with the environment variable QUICKJS_MODULES for searching the opencv.so module.

Usage
```
  import * as cv from 'opencv.so'

  let viewport = new cv.Size(640, 480);
  let mat = new cv.Mat(viewport, cv.CV_8UC3);
  let cap = new cv.VideoCapture(0);

  cap.read(mat);

  cv.imshow("qjs-opencv", mat);
```

Classes under namespace cv.*
* CLAHE - Contrast Limited Adaptive Histogram Equalization
* Contour - Iterable container based on a std::vector<cv::Point3d>
* Draw
* FastLineDetector
* Feature2D
* KeyPoint 
* Line
* LineSegmentDetector - 
* Mat - Object based on cv::Mat, calling mat(cv.Rect) makes a cv.Mat into an ROI
* MatIterator - yields TypedArray with offset 
* Point - cv::Point_<double>
* PointIterator - Iterator for ArrayBuffer/cv.Contour yielding a cv::Point_<double>
* Rect - cv::Rect_<double>
* RotatedRect - cv::RotatedRect3d
* Size - cv::Size_<double>
* SliceIterator - Partitions iterable into tuples or slides across an ArrayBuffer yielding offseted TypedArray
* Subdiv2D - cv::Subdiv2D base class for subdivision algorithms (Voronoi, etc.)
* UMat - cv::UMat, can be GPU cached texture for accelerated algorithms
* TickMeter - Provides a high-resolution timer object based on getTickCount/getTickFreq
* VideoCapture - Real-time camera input or video file playback
* VideoWriter - FFMPEG video writer


draw functions
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

highgui functions
* imshow
* namedWindow
* moveWindow
* resizeWindow
* destroyWindow
* getWindowImageRect
* getWindowProperty
* setWindowProperty
* setWindowTitle
* createTrackbar
* createButton
* getTrackbarPos
* setTrackbarPos
* setTrackbarMin
* setTrackbarMax
* getMouseWheelDelta
* setMouseCallback
* waitKey
* waitKeyEx
* displayOverlay

imgproc functions
* blur
* boundingRect
* GaussianBlur
* HoughLines
* HoughLinesP
* HoughCircles
* Canny
* goodFeaturesToTrack
* cvtColor
* equalizeHist
* threshold
* bilateralFilter
* findContours
* drawContours
* pointPolygonTest
* cornerHarris
* calcHist
* dilate
* erode
* morphologyEx
* medianBlur
* skeletonization
* pixelNeighborhood
* pixelNeighborhoodCross
* pixelFindValue
* paletteGenerate
* paletteApply
* paletteMatch
* accumulate
* accumulateProduct
* accumulateSquare
* accumulateWeighted
* createHanningWindow
* phaseCorrelate
* adaptiveThreshold
* blendLinear
* distanceTransform
* floodFill
* grabCut
* integral
* watershed
* applyColorMap
* moments
* convertMaps
* getAffineTransform
* getPerspectiveTransform
* getRectSubPix
* getRotationMatrix2D
* getRotationMatrix2D_
* invertAffineTransform
* linearPolar
* logPolar
* remap
* resize
* warpAffine
* warpPerspective
* warpPolar
* bilateralFilter
* blur
* boxFilter
* buildPyramid
* dilate
* erode
* filter2D
* GaussianBlur
* getDerivKernels
* getGaborKernel
* getGaussianKernel
* getStructuringElement
* Laplacian
* medianBlur
* morphologyDefaultBorderValue
* morphologyEx
* pyrDown
* pyrMeanShiftFiltering
* pyrUp
* Scharr
* sepFilter2D
* Sobel
* spatialGradient
* sqrBoxFilter
* approxPolyDP
* arcLength
* boxPoints
* connectedComponents
* connectedComponentsWithStats
* contourArea
* convexHull
* convexityDefects
* createGeneralizedHoughBallard
* createGeneralizedHoughGuil
* fitEllipse
* fitEllipseAMSYY
* fitEllipseDirect
* fitLine
* HuMoments
* intersectConvexConvex
* isContourConvex
* matchShapes
* minAreaRect
* minEnclosingCircle
* minEnclosingTriangle
* rotatedRectangleIntersection

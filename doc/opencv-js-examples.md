# opencv.js example scripts

Catalog of the 80 runnable example pages bundled with the OpenCV source (not from tutorial prose - the actual `<script id="codeSnippet">` blocks that ship in each page, the same code the online opencv.js tutorials embed as "Try it" iframes), grouped to match their tutorial category. Each entry lists every `cv.*` binding (function, constructor, or constant) the example's JS code references, extracted directly from the HTML.

**Source** (local checkout): `/mnt/data/Projects/opencv/doc/js_tutorials/js_assets/*.html`. The prose/theory for each topic lives one level up in the matching `js_tutorials/<category>/<topic>/*.markdown` file. DNN examples (`js_image_classification*`, `js_object_detection*`, `js_pose_estimation`, `js_semantic_segmentation`, `js_style_transfer`) share preprocessing logic from `js_assets/js_dnn_example_helper.js`, included below in their binding lists.

## Setup / GUI (`js_setup`, `js_gui`)

### `js_setup_usage.html`
Minimal end-to-end example: load an `<img>` into a `cv.Mat` and draw it to a `<canvas>`.

**Bindings:** `cv.Mat`, `cv.imread`, `cv.imshow`

### `js_image_display.html`
Read an image, convert to grayscale, and display both on canvases.

**Bindings:** `cv.COLOR_RGBA2GRAY`, `cv.Mat`, `cv.cvtColor`, `cv.imread`, `cv.imshow`

### `js_video_display.html`
Open a webcam via `cv.VideoCapture`, convert each frame to grayscale, and render it in a loop.

**Bindings:** `cv.COLOR_RGBA2GRAY`, `cv.CV_8UC1`, `cv.CV_8UC4`, `cv.Mat`, `cv.VideoCapture`, `cv.cvtColor`, `cv.imshow`

### `js_trackbar.html`
Blend two images with `cv.addWeighted()`, mixing ratio driven by an HTML range input.

**Bindings:** `cv.Mat`, `cv.addWeighted`, `cv.imread`, `cv.imshow`


## Core operations (`js_core`)

### `js_basic_ops_roi.html`
Copy a rectangular region of interest from one `cv.Mat` to another via `cv.Rect`.

**Bindings:** `cv.Mat`, `cv.Rect`, `cv.imread`, `cv.imshow`

### `js_basic_ops_copymakeborder.html`
Add a constant-color border around an image with `cv.copyMakeBorder()`.

**Bindings:** `cv.BORDER_CONSTANT`, `cv.Mat`, `cv.Scalar`, `cv.copyMakeBorder`, `cv.imread`, `cv.imshow`

### `js_image_arithmetics_bitwise.html`
Overlay a logo onto a photo using thresholding plus `cv.bitwise_and`/`cv.bitwise_not`/`cv.add`.

**Bindings:** `cv.COLOR_RGBA2GRAY`, `cv.Mat`, `cv.Rect`, `cv.THRESH_BINARY`, `cv.add`, `cv.bitwise_and`, `cv.bitwise_not`, `cv.cvtColor`, `cv.imread`, `cv.imshow`, `cv.threshold`


## Image processing - color & thresholding (`js_imgproc`)

### `js_colorspaces_cvtColor.html`
Convert an RGBA canvas image to grayscale with `cv.cvtColor()`.

**Bindings:** `cv.COLOR_RGBA2GRAY`, `cv.Mat`, `cv.cvtColor`, `cv.imread`, `cv.imshow`

### `js_colorspaces_inRange.html`
Mask pixels within a color range using `cv.inRange()`.

**Bindings:** `cv.Mat`, `cv.imread`, `cv.imshow`, `cv.inRange`

### `js_thresholding_threshold.html`
Apply a fixed-level binary threshold with `cv.threshold()`.

**Bindings:** `cv.Mat`, `cv.THRESH_BINARY`, `cv.imread`, `cv.imshow`, `cv.threshold`

### `js_thresholding_adaptiveThreshold.html`
Apply per-region adaptive thresholding with `cv.adaptiveThreshold()`.

**Bindings:** `cv.ADAPTIVE_THRESH_GAUSSIAN_C`, `cv.COLOR_RGBA2GRAY`, `cv.Mat`, `cv.THRESH_BINARY`, `cv.adaptiveThreshold`, `cv.cvtColor`, `cv.imread`, `cv.imshow`

### `js_histogram_begins_calcHist.html`
Compute and plot a grayscale histogram with `cv.calcHist()`.

**Bindings:** `cv.COLOR_RGBA2GRAY`, `cv.CV_8UC3`, `cv.FILLED`, `cv.Mat`, `cv.MatVector`, `cv.Point`, `cv.Scalar`, `cv.calcHist`, `cv.cvtColor`, `cv.imread`, `cv.imshow`, `cv.minMaxLoc`, `cv.rectangle`

### `js_histogram_equalization_equalizeHist.html`
Improve contrast globally via `cv.equalizeHist()`.

**Bindings:** `cv.COLOR_RGBA2GRAY`, `cv.Mat`, `cv.cvtColor`, `cv.equalizeHist`, `cv.imread`, `cv.imshow`

### `js_histogram_equalization_createCLAHE.html`
Improve local contrast with adaptive histogram equalization via `cv.CLAHE`.

**Bindings:** `cv.CLAHE`, `cv.COLOR_RGBA2GRAY`, `cv.Mat`, `cv.Size`, `cv.cvtColor`, `cv.equalizeHist`, `cv.imread`, `cv.imshow`

### `js_histogram_backprojection_calcBackProject.html`
Locate a region matching a reference histogram using `cv.calcBackProject()`.

**Bindings:** `cv.COLOR_RGB2HSV`, `cv.Mat`, `cv.MatVector`, `cv.NORM_MINMAX`, `cv.calcBackProject`, `cv.calcHist`, `cv.cvtColor`, `cv.imread`, `cv.imshow`, `cv.normalize`


## Image processing - filtering & morphology (`js_imgproc`)

### `js_filtering_blur.html`
Box-filter smoothing via `cv.blur()`/`cv.boxFilter()`.

**Bindings:** `cv.BORDER_DEFAULT`, `cv.Mat`, `cv.Point`, `cv.Size`, `cv.blur`, `cv.boxFilter`, `cv.imread`, `cv.imshow`

### `js_filtering_GaussianBlur.html`
Gaussian smoothing via `cv.GaussianBlur()`.

**Bindings:** `cv.BORDER_DEFAULT`, `cv.GaussianBlur`, `cv.Mat`, `cv.Size`, `cv.imread`, `cv.imshow`

### `js_filtering_medianBlur.html`
Salt-and-pepper noise removal via `cv.medianBlur()`.

**Bindings:** `cv.Mat`, `cv.imread`, `cv.imshow`, `cv.medianBlur`

### `js_filtering_bilateralFilter.html`
Edge-preserving smoothing via `cv.bilateralFilter()`.

**Bindings:** `cv.BORDER_DEFAULT`, `cv.COLOR_RGBA2RGB`, `cv.Mat`, `cv.bilateralFilter`, `cv.cvtColor`, `cv.imread`, `cv.imshow`

### `js_filtering_filter.html`
Apply an arbitrary convolution kernel with `cv.filter2D()`.

**Bindings:** `cv.BORDER_DEFAULT`, `cv.CV_32FC1`, `cv.CV_8U`, `cv.Mat`, `cv.Point`, `cv.filter2D`, `cv.imread`, `cv.imshow`

### `js_morphological_ops_erode.html`
Shrink bright regions with `cv.erode()`.

**Bindings:** `cv.BORDER_CONSTANT`, `cv.CV_8U`, `cv.Mat`, `cv.Point`, `cv.erode`, `cv.imread`, `cv.imshow`, `cv.morphologyDefaultBorderValue`

### `js_morphological_ops_dilate.html`
Grow bright regions with `cv.dilate()`.

**Bindings:** `cv.BORDER_CONSTANT`, `cv.CV_8U`, `cv.Mat`, `cv.Point`, `cv.dilate`, `cv.imread`, `cv.imshow`, `cv.morphologyDefaultBorderValue`

### `js_morphological_ops_opening.html`
Remove small bright noise via opening (`cv.morphologyEx` + `MORPH_OPEN`).

**Bindings:** `cv.BORDER_CONSTANT`, `cv.CV_8U`, `cv.MORPH_OPEN`, `cv.Mat`, `cv.Point`, `cv.imread`, `cv.imshow`, `cv.morphologyDefaultBorderValue`, `cv.morphologyEx`

### `js_morphological_ops_closing.html`
Fill small dark holes via closing (`cv.morphologyEx` + `MORPH_CLOSE`).

**Bindings:** `cv.CV_8U`, `cv.MORPH_CLOSE`, `cv.Mat`, `cv.imread`, `cv.imshow`, `cv.morphologyEx`

### `js_morphological_ops_gradient.html`
Outline shapes via morphological gradient (`cv.morphologyEx` + `MORPH_GRADIENT`).

**Bindings:** `cv.COLOR_RGBA2RGB`, `cv.CV_8U`, `cv.MORPH_GRADIENT`, `cv.Mat`, `cv.cvtColor`, `cv.imread`, `cv.imshow`, `cv.morphologyEx`

### `js_morphological_ops_topHat.html`
Extract small bright details via top-hat (`cv.morphologyEx` + `MORPH_TOPHAT`).

**Bindings:** `cv.COLOR_RGBA2RGB`, `cv.CV_8U`, `cv.MORPH_TOPHAT`, `cv.Mat`, `cv.cvtColor`, `cv.imread`, `cv.imshow`, `cv.morphologyEx`

### `js_morphological_ops_blackHat.html`
Extract small dark details via black-hat (`cv.morphologyEx` + `MORPH_BLACKHAT`).

**Bindings:** `cv.COLOR_RGBA2RGB`, `cv.CV_8U`, `cv.MORPH_BLACKHAT`, `cv.Mat`, `cv.cvtColor`, `cv.imread`, `cv.imshow`, `cv.morphologyEx`

### `js_morphological_ops_getStructuringElement.html`
Build a custom structuring-element kernel with `cv.getStructuringElement()`.

**Bindings:** `cv.COLOR_RGBA2RGB`, `cv.MORPH_CROSS`, `cv.MORPH_GRADIENT`, `cv.Mat`, `cv.Size`, `cv.cvtColor`, `cv.getStructuringElement`, `cv.imread`, `cv.imshow`, `cv.morphologyEx`

### `js_gradients_Sobel.html`
Edge gradients via `cv.Sobel()`/`cv.Scharr()`.

**Bindings:** `cv.BORDER_DEFAULT`, `cv.COLOR_RGB2GRAY`, `cv.CV_8U`, `cv.Mat`, `cv.Scharr`, `cv.Sobel`, `cv.cvtColor`, `cv.imread`, `cv.imshow`

### `js_gradients_Laplacian.html`
Second-derivative edge detection via `cv.Laplacian()`.

**Bindings:** `cv.BORDER_DEFAULT`, `cv.COLOR_RGB2GRAY`, `cv.CV_8U`, `cv.Laplacian`, `cv.Mat`, `cv.cvtColor`, `cv.imread`, `cv.imshow`

### `js_gradients_absSobel.html`
Sobel gradient converted back to a displayable 8-bit image via `cv.convertScaleAbs()`.

**Bindings:** `cv.BORDER_DEFAULT`, `cv.COLOR_RGB2GRAY`, `cv.CV_64F`, `cv.CV_8U`, `cv.Mat`, `cv.Sobel`, `cv.convertScaleAbs`, `cv.cvtColor`, `cv.imread`, `cv.imshow`

### `js_canny.html`
Canny edge detection via `cv.Canny()`.

**Bindings:** `cv.COLOR_RGB2GRAY`, `cv.Canny`, `cv.Mat`, `cv.cvtColor`, `cv.imread`, `cv.imshow`

### `js_pyramids_pyrDown.html`
Downsample (blur + halve) an image with `cv.pyrDown()`.

**Bindings:** `cv.BORDER_DEFAULT`, `cv.Mat`, `cv.Size`, `cv.imread`, `cv.imshow`, `cv.pyrDown`

### `js_pyramids_pyrUp.html`
Upsample (double + blur) an image with `cv.pyrUp()`.

**Bindings:** `cv.BORDER_DEFAULT`, `cv.Mat`, `cv.Size`, `cv.imread`, `cv.imshow`, `cv.pyrUp`


## Image processing - geometric transforms (`js_imgproc`)

### `js_geometric_transformations_resize.html`
Resize an image with `cv.resize()`.

**Bindings:** `cv.INTER_AREA`, `cv.Mat`, `cv.Size`, `cv.imread`, `cv.imshow`, `cv.resize`

### `js_geometric_transformations_warpAffine.html`
Apply an arbitrary affine transform with `cv.warpAffine()`.

**Bindings:** `cv.BORDER_CONSTANT`, `cv.CV_64FC1`, `cv.INTER_LINEAR`, `cv.Mat`, `cv.Scalar`, `cv.Size`, `cv.imread`, `cv.imshow`, `cv.matFromArray`, `cv.warpAffine`

### `js_geometric_transformations_rotateWarpAffine.html`
Rotate an image about a center point via `cv.getRotationMatrix2D()` + `cv.warpAffine()`.

**Bindings:** `cv.BORDER_CONSTANT`, `cv.INTER_LINEAR`, `cv.Mat`, `cv.Point`, `cv.Scalar`, `cv.Size`, `cv.getRotationMatrix2D`, `cv.imread`, `cv.imshow`, `cv.warpAffine`

### `js_geometric_transformations_getAffineTransform.html`
Derive an affine matrix from 3 point correspondences with `cv.getAffineTransform()`.

**Bindings:** `cv.BORDER_CONSTANT`, `cv.CV_32FC2`, `cv.INTER_LINEAR`, `cv.Mat`, `cv.Scalar`, `cv.Size`, `cv.getAffineTransform`, `cv.imread`, `cv.imshow`, `cv.matFromArray`, `cv.warpAffine`

### `js_geometric_transformations_warpPerspective.html`
Apply a perspective (homography) warp via `cv.getPerspectiveTransform()` + `cv.warpPerspective()`.

**Bindings:** `cv.BORDER_CONSTANT`, `cv.CV_32FC2`, `cv.INTER_LINEAR`, `cv.Mat`, `cv.Scalar`, `cv.Size`, `cv.getPerspectiveTransform`, `cv.imread`, `cv.imshow`, `cv.matFromArray`, `cv.warpPerspective`


## Image processing - shape detection (`js_imgproc`)

### `js_houghlines_HoughLines.html`
Detect straight lines (standard Hough transform) via `cv.HoughLines()`.

**Bindings:** `cv.COLOR_RGBA2GRAY`, `cv.CV_8UC3`, `cv.Canny`, `cv.HoughLines`, `cv.Mat`, `cv.cvtColor`, `cv.imread`, `cv.imshow`, `cv.line`

### `js_houghlines_HoughLinesP.html`
Detect line segments (probabilistic Hough transform) via `cv.HoughLinesP()`.

**Bindings:** `cv.COLOR_RGBA2GRAY`, `cv.CV_8UC3`, `cv.Canny`, `cv.HoughLinesP`, `cv.Mat`, `cv.Point`, `cv.Scalar`, `cv.cvtColor`, `cv.imread`, `cv.imshow`, `cv.line`

### `js_houghcircles_HoughCirclesP.html`
Detect circles via `cv.HoughCircles()`.

**Bindings:** `cv.COLOR_RGBA2GRAY`, `cv.CV_8U`, `cv.HOUGH_GRADIENT`, `cv.HoughCircles`, `cv.Mat`, `cv.Point`, `cv.Scalar`, `cv.circle`, `cv.cvtColor`, `cv.imread`, `cv.imshow`

### `js_template_matching_matchTemplate.html`
Locate a template sub-image via `cv.matchTemplate()` + `cv.minMaxLoc()`.

**Bindings:** `cv.LINE_8`, `cv.Mat`, `cv.Point`, `cv.Scalar`, `cv.TM_CCOEFF`, `cv.imread`, `cv.imshow`, `cv.matchTemplate`, `cv.minMaxLoc`, `cv.rectangle`


## Image processing - contours (`js_imgproc`)

### `js_contours_begin_contours.html`
Find and draw contours via `cv.findContours()`/`cv.drawContours()`.

**Bindings:** `cv.CHAIN_APPROX_SIMPLE`, `cv.COLOR_RGBA2GRAY`, `cv.CV_8UC3`, `cv.LINE_8`, `cv.Mat`, `cv.MatVector`, `cv.RETR_CCOMP`, `cv.Scalar`, `cv.THRESH_BINARY`, `cv.cvtColor`, `cv.drawContours`, `cv.findContours`, `cv.imread`, `cv.imshow`, `cv.threshold`

### `js_contour_features_area.html`
Contour area via `cv.contourArea()`.

**Bindings:** `cv.CHAIN_APPROX_SIMPLE`, `cv.COLOR_RGBA2GRAY`, `cv.CV_8UC3`, `cv.Mat`, `cv.MatVector`, `cv.RETR_CCOMP`, `cv.THRESH_BINARY`, `cv.contourArea`, `cv.cvtColor`, `cv.findContours`, `cv.imread`, `cv.threshold`

### `js_contour_features_perimeter.html`
Contour perimeter (arc length) via `cv.arcLength()`.

**Bindings:** `cv.CHAIN_APPROX_SIMPLE`, `cv.COLOR_RGBA2GRAY`, `cv.CV_8UC3`, `cv.Mat`, `cv.MatVector`, `cv.RETR_CCOMP`, `cv.THRESH_BINARY`, `cv.arcLength`, `cv.cvtColor`, `cv.findContours`, `cv.imread`, `cv.threshold`

### `js_contour_features_approxPolyDP.html`
Polygon approximation of a contour via `cv.approxPolyDP()`.

**Bindings:** `cv.CHAIN_APPROX_SIMPLE`, `cv.COLOR_RGBA2GRAY`, `cv.CV_8UC3`, `cv.Mat`, `cv.MatVector`, `cv.RETR_CCOMP`, `cv.Scalar`, `cv.THRESH_BINARY`, `cv.approxPolyDP`, `cv.cvtColor`, `cv.drawContours`, `cv.findContours`, `cv.imread`, `cv.imshow`, `cv.threshold`

### `js_contour_features_convexHull.html`
Convex hull of a contour via `cv.convexHull()`.

**Bindings:** `cv.CHAIN_APPROX_SIMPLE`, `cv.COLOR_RGBA2GRAY`, `cv.CV_8UC3`, `cv.Mat`, `cv.MatVector`, `cv.RETR_CCOMP`, `cv.Scalar`, `cv.THRESH_BINARY`, `cv.convexHull`, `cv.cvtColor`, `cv.drawContours`, `cv.findContours`, `cv.imread`, `cv.imshow`, `cv.threshold`

### `js_contour_features_boundingRect.html`
Axis-aligned bounding box of a contour via `cv.boundingRect()`.

**Bindings:** `cv.CHAIN_APPROX_SIMPLE`, `cv.COLOR_RGBA2GRAY`, `cv.CV_8UC3`, `cv.LINE_AA`, `cv.Mat`, `cv.MatVector`, `cv.Point`, `cv.RETR_CCOMP`, `cv.Scalar`, `cv.THRESH_BINARY`, `cv.boundingRect`, `cv.cvtColor`, `cv.drawContours`, `cv.findContours`, `cv.imread`, `cv.imshow`, `cv.rectangle`, `cv.threshold`

### `js_contour_features_minAreaRect.html`
Minimum-area rotated bounding box via `cv.minAreaRect()`.

**Bindings:** `cv.CHAIN_APPROX_SIMPLE`, `cv.COLOR_RGBA2GRAY`, `cv.CV_8UC3`, `cv.LINE_AA`, `cv.Mat`, `cv.MatVector`, `cv.RETR_CCOMP`, `cv.RotatedRect`, `cv.Scalar`, `cv.THRESH_BINARY`, `cv.cvtColor`, `cv.drawContours`, `cv.findContours`, `cv.imread`, `cv.imshow`, `cv.line`, `cv.minAreaRect`, `cv.threshold`

### `js_contour_features_minEnclosingCircle.html`
Minimum enclosing circle of a contour via `cv.minEnclosingCircle()`.

**Bindings:** `cv.CHAIN_APPROX_SIMPLE`, `cv.COLOR_RGBA2GRAY`, `cv.CV_8UC3`, `cv.Mat`, `cv.MatVector`, `cv.RETR_CCOMP`, `cv.Scalar`, `cv.THRESH_BINARY`, `cv.circle`, `cv.cvtColor`, `cv.drawContours`, `cv.findContours`, `cv.imread`, `cv.imshow`, `cv.minEnclosingCircle`, `cv.threshold`

### `js_contour_features_fitEllipse.html`
Best-fit ellipse to a contour via `cv.fitEllipse()`.

**Bindings:** `cv.CHAIN_APPROX_SIMPLE`, `cv.COLOR_RGBA2GRAY`, `cv.CV_8UC3`, `cv.LINE_8`, `cv.Mat`, `cv.MatVector`, `cv.RETR_CCOMP`, `cv.Scalar`, `cv.THRESH_BINARY`, `cv.cvtColor`, `cv.drawContours`, `cv.ellipse1`, `cv.findContours`, `cv.fitEllipse`, `cv.imread`, `cv.imshow`, `cv.threshold`

### `js_contour_features_fitLine.html`
Best-fit line through a contour via `cv.fitLine()`.

**Bindings:** `cv.CHAIN_APPROX_SIMPLE`, `cv.COLOR_RGBA2GRAY`, `cv.CV_8UC3`, `cv.DIST_L2`, `cv.LINE_AA`, `cv.Mat`, `cv.MatVector`, `cv.Point`, `cv.RETR_CCOMP`, `cv.Scalar`, `cv.THRESH_BINARY`, `cv.cvtColor`, `cv.drawContours`, `cv.findContours`, `cv.fitLine`, `cv.imread`, `cv.imshow`, `cv.line`, `cv.threshold`

### `js_contour_features_moments.html`
Image moments (centroid, etc.) via `cv.moments()`.

**Bindings:** `cv.CHAIN_APPROX_SIMPLE`, `cv.COLOR_RGBA2GRAY`, `cv.CV_8UC3`, `cv.Mat`, `cv.MatVector`, `cv.RETR_CCOMP`, `cv.THRESH_BINARY`, `cv.cvtColor`, `cv.findContours`, `cv.imread`, `cv.moments`, `cv.threshold`

### `js_contour_properties_transpose.html`
Transpose an image/mask with `cv.transpose()`.

**Bindings:** `cv.COLOR_RGBA2GRAY`, `cv.Mat`, `cv.THRESH_BINARY`, `cv.cvtColor`, `cv.imread`, `cv.imshow`, `cv.threshold`, `cv.transpose`

### `js_contours_more_functions_convexityDefects.html`
Find dents in a contour relative to its convex hull via `cv.convexityDefects()`.

**Bindings:** `cv.CHAIN_APPROX_SIMPLE`, `cv.COLOR_RGBA2GRAY`, `cv.CV_8UC3`, `cv.LINE_AA`, `cv.Mat`, `cv.MatVector`, `cv.Point`, `cv.RETR_CCOMP`, `cv.Scalar`, `cv.THRESH_BINARY`, `cv.circle`, `cv.convexHull`, `cv.convexityDefects`, `cv.cvtColor`, `cv.findContours`, `cv.imread`, `cv.imshow`, `cv.line`, `cv.threshold`

### `js_contours_more_functions_shape.html`
Compare two contour shapes via `cv.matchShapes()`.

**Bindings:** `cv.CHAIN_APPROX_SIMPLE`, `cv.COLOR_RGBA2GRAY`, `cv.CV_8UC3`, `cv.LINE_8`, `cv.Mat`, `cv.MatVector`, `cv.RETR_CCOMP`, `cv.Scalar`, `cv.THRESH_BINARY`, `cv.cvtColor`, `cv.drawContours`, `cv.findContours`, `cv.imread`, `cv.imshow`, `cv.matchShapes`, `cv.threshold`


## Image processing - segmentation & misc (`js_imgproc`)

### `js_watershed_threshold.html`
Otsu-threshold an image as the first step of the watershed pipeline.

**Bindings:** `cv.COLOR_RGBA2GRAY`, `cv.Mat`, `cv.THRESH_BINARY_INV`, `cv.THRESH_OTSU`, `cv.cvtColor`, `cv.imread`, `cv.imshow`, `cv.threshold`

### `js_watershed_background.html`
Estimate the sure-background region via dilation for watershed segmentation.

**Bindings:** `cv.COLOR_RGBA2GRAY`, `cv.CV_8U`, `cv.Mat`, `cv.Point`, `cv.THRESH_BINARY_INV`, `cv.THRESH_OTSU`, `cv.cvtColor`, `cv.dilate`, `cv.erode`, `cv.imread`, `cv.imshow`, `cv.threshold`

### `js_watershed_distanceTransform.html`
Estimate the sure-foreground region via `cv.distanceTransform()`.

**Bindings:** `cv.COLOR_RGBA2GRAY`, `cv.CV_8U`, `cv.DIST_L2`, `cv.Mat`, `cv.NORM_INF`, `cv.Point`, `cv.THRESH_BINARY_INV`, `cv.THRESH_OTSU`, `cv.cvtColor`, `cv.dilate`, `cv.distanceTransform`, `cv.erode`, `cv.imread`, `cv.imshow`, `cv.normalize`, `cv.threshold`

### `js_watershed_foreground.html`
Derive sure-foreground markers from the distance transform's peak regions.

**Bindings:** `cv.COLOR_RGBA2GRAY`, `cv.CV_8U`, `cv.DIST_L2`, `cv.Mat`, `cv.NORM_INF`, `cv.Point`, `cv.THRESH_BINARY`, `cv.THRESH_BINARY_INV`, `cv.THRESH_OTSU`, `cv.cvtColor`, `cv.dilate`, `cv.distanceTransform`, `cv.erode`, `cv.imread`, `cv.imshow`, `cv.normalize`, `cv.threshold`

### `js_watershed_watershed.html`
Segment touching objects with marker-based `cv.watershed()`.

**Bindings:** `cv.COLOR_RGBA2GRAY`, `cv.COLOR_RGBA2RGB`, `cv.CV_8U`, `cv.DIST_L2`, `cv.Mat`, `cv.NORM_INF`, `cv.Point`, `cv.THRESH_BINARY`, `cv.THRESH_BINARY_INV`, `cv.THRESH_OTSU`, `cv.connectedComponents`, `cv.cvtColor`, `cv.dilate`, `cv.distanceTransform`, `cv.erode`, `cv.imread`, `cv.imshow`, `cv.normalize`, `cv.subtract`, `cv.threshold`, `cv.watershed`

### `js_grabcut_grabCut.html`
Interactive foreground extraction via `cv.grabCut()`.

**Bindings:** `cv.COLOR_RGBA2RGB`, `cv.GC_INIT_WITH_RECT`, `cv.Mat`, `cv.Point`, `cv.Rect`, `cv.Scalar`, `cv.cvtColor`, `cv.grabCut`, `cv.imread`, `cv.imshow`, `cv.rectangle`

### `js_fourier_transform_dft.html`
Frequency-domain analysis via `cv.dft()` (magnitude spectrum).

**Bindings:** `cv.BORDER_CONSTANT`, `cv.COLOR_RGBA2GRAY`, `cv.CV_32F`, `cv.CV_32S`, `cv.Mat`, `cv.MatVector`, `cv.NORM_MINMAX`, `cv.Rect`, `cv.Scalar`, `cv.add`, `cv.copyMakeBorder`, `cv.cvtColor`, `cv.dft`, `cv.getOptimalDFTSize`, `cv.imread`, `cv.imshow`, `cv.log`, `cv.magnitude`, `cv.merge`, `cv.normalize`, `cv.split`

### `js_intelligent_scissors.html`
Interactive live-wire segmentation via `cv.segmentation_IntelligentScissorsMB`.

**Bindings:** `cv.LINE_8`, `cv.Mat`, `cv.MatVector`, `cv.Point`, `cv.Scalar`, `cv.Size`, `cv.imread`, `cv.imshow`, `cv.polylines`, `cv.resize`, `cv.segmentation_IntelligentScissorsMB`

### `js_imgproc_camera.html`
Live-camera playground cycling through grayscale/HSV/Canny/inRange/threshold on webcam frames.

**Bindings:** `cv.ADAPTIVE_THRESH_GAUSSIAN_C`, `cv.BORDER_CONSTANT`, `cv.BORDER_DEFAULT`, `cv.BORDER_REFLECT`, `cv.BORDER_REFLECT_101`, `cv.BORDER_REPLICATE`, `cv.CHAIN_APPROX_NONE`, `cv.CHAIN_APPROX_SIMPLE`, `cv.CHAIN_APPROX_TC89_KCOS`, `cv.CHAIN_APPROX_TC89_L1`, `cv.COLOR_GRAY2RGBA`, `cv.COLOR_RGB2GRAY`, `cv.COLOR_RGB2HSV`, `cv.COLOR_RGBA2GRAY`, `cv.COLOR_RGBA2RGB`, `cv.CV_8U`, `cv.CV_8UC1`, `cv.CV_8UC3`, `cv.CV_8UC4`, `cv.Canny`, `cv.FILLED`, `cv.GaussianBlur`, `cv.LINE_8`, `cv.Laplacian`, `cv.MORPH_BLACKHAT`, `cv.MORPH_CLOSE`, `cv.MORPH_CROSS`, `cv.MORPH_DILATE`, `cv.MORPH_ELLIPSE`, `cv.MORPH_ERODE`, `cv.MORPH_GRADIENT`, `cv.MORPH_OPEN`, `cv.MORPH_RECT`, `cv.MORPH_TOPHAT`, `cv.Mat`, `cv.MatVector`, `cv.NORM_MINMAX`, `cv.RETR_CCOMP`, `cv.RETR_EXTERNAL`, `cv.RETR_LIST`, `cv.RETR_TREE`, `cv.Scalar`, `cv.Scharr`, `cv.Sobel`, `cv.THRESH_BINARY`, `cv.VideoCapture`, `cv.adaptiveThreshold`, `cv.bilateralFilter`, `cv.calcBackProject`, `cv.calcHist`, `cv.cvtColor`, `cv.dilate`, `cv.drawContours`, `cv.equalizeHist`, `cv.erode`, `cv.findContours`, `cv.getStructuringElement`, `cv.imshow`, `cv.inRange`, `cv.medianBlur`, `cv.minMaxLoc`, `cv.morphologyEx`, `cv.normalize`, `cv.rectangle`, `cv.threshold`


## Video analysis (`js_video`)

### `js_bg_subtraction.html`
Foreground/background segmentation on video via `cv.BackgroundSubtractorMOG2`.

**Bindings:** `cv.BackgroundSubtractorMOG2`, `cv.CV_8UC1`, `cv.CV_8UC4`, `cv.Mat`, `cv.VideoCapture`, `cv.imshow`

### `js_meanshift.html`
Track an object's location (fixed window size) with `cv.meanShift()`.

**Bindings:** `cv.COLOR_RGB2HSV`, `cv.COLOR_RGBA2RGB`, `cv.CV_8UC3`, `cv.CV_8UC4`, `cv.Mat`, `cv.MatVector`, `cv.NORM_MINMAX`, `cv.Point`, `cv.Rect`, `cv.Scalar`, `cv.TERM_CRITERIA_COUNT`, `cv.TERM_CRITERIA_EPS`, `cv.TermCriteria`, `cv.VideoCapture`, `cv.calcBackProject`, `cv.calcHist`, `cv.cvtColor`, `cv.imshow`, `cv.inRange`, `cv.meanShift`, `cv.normalize`, `cv.rectangle`

### `js_camshift.html`
Track an object with an adaptively-resizing window via `cv.CamShift()`.

**Bindings:** `cv.COLOR_RGB2HSV`, `cv.COLOR_RGBA2RGB`, `cv.CV_8UC3`, `cv.CV_8UC4`, `cv.CamShift`, `cv.Mat`, `cv.MatVector`, `cv.NORM_MINMAX`, `cv.Rect`, `cv.Scalar`, `cv.TERM_CRITERIA_COUNT`, `cv.TERM_CRITERIA_EPS`, `cv.TermCriteria`, `cv.VideoCapture`, `cv.calcBackProject`, `cv.calcHist`, `cv.cvtColor`, `cv.imshow`, `cv.inRange`, `cv.line`, `cv.normalize`, `cv.rotatedRectPoints`

### `js_optical_flow_lucas_kanade.html`
Sparse feature tracking via `cv.calcOpticalFlowPyrLK()` + `cv.goodFeaturesToTrack()`.

**Bindings:** `cv.COLOR_RGB2GRAY`, `cv.COLOR_RGBA2GRAY`, `cv.CV_32FC2`, `cv.CV_8UC4`, `cv.Mat`, `cv.Point`, `cv.Scalar`, `cv.Size`, `cv.TERM_CRITERIA_COUNT`, `cv.TERM_CRITERIA_EPS`, `cv.TermCriteria`, `cv.VideoCapture`, `cv.add`, `cv.calcOpticalFlowPyrLK`, `cv.circle`, `cv.cvtColor`, `cv.goodFeaturesToTrack`, `cv.imshow`, `cv.line`

### `js_optical_flow_dense.html`
Dense per-pixel motion field via `cv.calcOpticalFlowFarneback()`, visualized as HSV.

**Bindings:** `cv.COLOR_HSV2RGB`, `cv.COLOR_RGBA2GRAY`, `cv.CV_32FC1`, `cv.CV_32FC2`, `cv.CV_8UC1`, `cv.CV_8UC3`, `cv.CV_8UC4`, `cv.Mat`, `cv.MatVector`, `cv.NORM_MINMAX`, `cv.Scalar`, `cv.VideoCapture`, `cv.calcOpticalFlowFarneback`, `cv.cartToPolar`, `cv.cvtColor`, `cv.imshow`, `cv.merge`, `cv.normalize`, `cv.split`


## Deep learning (`js_dnn`)

### `js_image_classification.html`
Classify an image with a pretrained ONNX/Caffe/TF model loaded via `cv.readNet()`.

**Bindings:** `cv.COLOR_RGBA2BGR`, `cv.CV_8UC3`, `cv.FS_createDataFile`, `cv.Mat`, `cv.Scalar`, `cv.Size`, `cv.blobFromImage`, `cv.cvtColor`, `cv.imread`, `cv.readNet`

### `js_image_classification_webnn_polyfill.html`
Same image-classification pipeline, using the WebNN polyfill backend.

**Bindings:** `cv.COLOR_RGBA2BGR`, `cv.CV_8UC3`, `cv.FS_createDataFile`, `cv.Mat`, `cv.Scalar`, `cv.Size`, `cv.blobFromImage`, `cv.cvtColor`, `cv.imread`, `cv.readNet`

### `js_image_classification_with_camera.html`
Live webcam image classification.

**Bindings:** `cv.COLOR_RGBA2BGR`, `cv.CV_8UC3`, `cv.CV_8UC4`, `cv.FS_createDataFile`, `cv.Mat`, `cv.Scalar`, `cv.Size`, `cv.VideoCapture`, `cv.blobFromImage`, `cv.cvtColor`, `cv.imread`, `cv.readNet`

### `webnn-electron/js_image_classification_webnn_electron.html`
Image classification inside an Electron app using native WebNN.

**Bindings:** `cv.COLOR_RGBA2BGR`, `cv.CV_8UC3`, `cv.FS_createDataFile`, `cv.Mat`, `cv.Scalar`, `cv.Size`, `cv.blobFromImage`, `cv.cvtColor`, `cv.imread`, `cv.readNet`

### `js_object_detection.html`
Detect and box objects in an image with a pretrained detection network.

**Bindings:** `cv.COLOR_RGBA2BGR`, `cv.COLOR_RGBA2RGB`, `cv.CV_8UC3`, `cv.FILLED`, `cv.FONT_HERSHEY_SIMPLEX`, `cv.FS_createDataFile`, `cv.Mat`, `cv.Point`, `cv.Scalar`, `cv.Size`, `cv.blobFromImage`, `cv.cvtColor`, `cv.imread`, `cv.putText`, `cv.readNet`, `cv.rectangle`

### `js_object_detection_with_camera.html`
Live webcam object detection.

**Bindings:** `cv.COLOR_RGBA2BGR`, `cv.COLOR_RGBA2RGB`, `cv.CV_8UC3`, `cv.CV_8UC4`, `cv.FILLED`, `cv.FONT_HERSHEY_SIMPLEX`, `cv.FS_createDataFile`, `cv.Mat`, `cv.Point`, `cv.Scalar`, `cv.Size`, `cv.VideoCapture`, `cv.blobFromImage`, `cv.cvtColor`, `cv.imread`, `cv.putText`, `cv.readNet`, `cv.rectangle`

### `js_pose_estimation.html`
Estimate human pose keypoints with a pretrained network.

**Bindings:** `cv.COLOR_RGBA2BGR`, `cv.COLOR_RGBA2RGB`, `cv.CV_8UC3`, `cv.FILLED`, `cv.FS_createDataFile`, `cv.Mat`, `cv.Point`, `cv.Scalar`, `cv.Size`, `cv.blobFromImage`, `cv.cvtColor`, `cv.ellipse`, `cv.imread`, `cv.line`, `cv.readNet`

### `js_semantic_segmentation.html`
Per-pixel class segmentation with a pretrained network.

**Bindings:** `cv.COLOR_RGBA2BGR`, `cv.CV_8UC3`, `cv.CV_8UC4`, `cv.FS_createDataFile`, `cv.Mat`, `cv.Scalar`, `cv.Size`, `cv.blobFromImage`, `cv.cvtColor`, `cv.imread`, `cv.matFromArray`, `cv.readNet`

### `js_style_transfer.html`
Apply neural style transfer to an image with a pretrained network.

**Bindings:** `cv.COLOR_RGBA2BGR`, `cv.CV_8UC3`, `cv.CV_8UC4`, `cv.FS_createDataFile`, `cv.Mat`, `cv.Scalar`, `cv.Size`, `cv.blobFromImage`, `cv.cvtColor`, `cv.imread`, `cv.matFromArray`, `cv.readNet`


/*
 * Skeletonization.hpp
 *
 *  Created on: Oct 2, 2014
 *      Author: Raghu
 *
 *  Code for thinning a binary image using Zhang-Suen algorithm.
 */

#ifndef SKELETONIZATION_HPP
#define SKELETONIZATION_HPP

#include <iostream>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

/**
 * Perform one thinning iteration.
 * Normally you wouldn't call this function directly from your code.
 */
void
thinning_iteration(cv::Mat& im, int iter) {
  cv::Mat marker = cv::Mat::zeros(im.size(), CV_8UC1);

  for(int i = 1; i < im.rows - 1; i++) {
    for(int j = 1; j < im.cols - 1; j++) {
      uchar p2 = im.at<uchar>(i - 1, j);
      uchar p3 = im.at<uchar>(i - 1, j + 1);
      uchar p4 = im.at<uchar>(i, j + 1);
      uchar p5 = im.at<uchar>(i + 1, j + 1);
      uchar p6 = im.at<uchar>(i + 1, j);
      uchar p7 = im.at<uchar>(i + 1, j - 1);
      uchar p8 = im.at<uchar>(i, j - 1);
      uchar p9 = im.at<uchar>(i - 1, j - 1);

      int A = (p2 == 0 && p3 == 1) + (p3 == 0 && p4 == 1) + (p4 == 0 && p5 == 1) + (p5 == 0 && p6 == 1) + (p6 == 0 && p7 == 1) +
              (p7 == 0 && p8 == 1) + (p8 == 0 && p9 == 1) + (p9 == 0 && p2 == 1);
      int B = p2 + p3 + p4 + p5 + p6 + p7 + p8 + p9;
      int m1 = iter == 0 ? (p2 * p4 * p6) : (p2 * p4 * p8);
      int m2 = iter == 0 ? (p4 * p6 * p8) : (p2 * p6 * p8);

      if(A == 1 && (B >= 2 && B <= 6) && m1 == 0 && m2 == 0)
        marker.at<uchar>(i, j) = 1;
    }
  }

  im &= ~marker;
}

/**
 * Function for thinning the given binary image
 */
void
thinning(cv::Mat& im) {
  im /= 255;

  cv::Mat prev = cv::Mat::zeros(im.size(), CV_8UC1);
  cv::Mat diff;

  do {
    thinning_iteration(im, 0);
    thinning_iteration(im, 1);
    cv::absdiff(im, prev, diff);
    im.copyTo(prev);
  } while(cv::countNonZero(diff) > 0);

  im *= 255;
}

/**
 * This is the function that acts as the input/output system of this header file.
 */
template<class InputArray>
cv::Mat
skeletonization(InputArray inputImage) {
  if(inputImage.empty())
    std::cout << "Inside skeletonization, Source empty" << std::endl;

  cv::Mat outputImage;

  if(inputImage.channels() == 1)
    inputImage.copyTo(outputImage);
  else
    cv::cvtColor(inputImage, outputImage, cv::COLOR_BGR2GRAY);

  cv::threshold(outputImage, outputImage, 0, 255, cv::THRESH_BINARY + cv::THRESH_OTSU);

  thinning(outputImage);

  return outputImage;
}

#endif /* SKELETONIZATION_HPP */

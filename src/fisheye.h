#ifndef FISHEYE_H
#define FISHEYE_H
#include <opencv2/opencv.hpp>

cv::Mat undistortImage(const cv::Mat& distorted, cv::Mat K, cv::Mat D, cv::Size corr_ori, double balance);

#endif

#ifndef FISHEYE_H
#define FISHEYE_H
#include <opencv2/opencv.hpp>

void precomputeUndistortMaps(const cv::Mat& K, const cv::Mat& D, const cv::Size& imageSize, double balance, cv::Mat& map1, cv::Mat& map2);
cv::Mat undistortImage(const cv::Mat& distorted, cv::Mat K, cv::Mat D, cv::Size corr_ori, double balance);

#endif

#include "fisheye.h"

void precomputeUndistortMaps(const cv::Mat& K, const cv::Mat& D, const cv::Size& imageSize, double balance, cv::Mat& map1, cv::Mat& map2) {
    // Estimate the new camera matrix for undistortion
    cv::Mat new_K;
    cv::fisheye::estimateNewCameraMatrixForUndistortRectify(
        K, D, imageSize, cv::Matx33d::eye(), new_K, balance
    );
    
    // Initialize the undistort rectify map
    cv::fisheye::initUndistortRectifyMap(
       K, D, cv::Matx33d::eye(), new_K, imageSize, CV_32FC1, map1, map2
    );
}   


cv::Mat undistortImage(const cv::Mat& distorted, cv::Mat K, cv::Mat D, cv::Size corr_ori, double balance )
{

    // Get the dimensions of the input image
    cv::Size dim1(distorted.cols, distorted.rows);

    // Ensure the aspect ratio matches the calibration
    double aspect_ratio_input = static_cast<double>(dim1.width) / dim1.height;
    double aspect_ratio_calib = aspect_ratio_input;

    // Set dim2 and dim3 to dim1 if not provided
    cv::Size dim2 = dim1;
    cv::Size dim3 = dim1;

    // Scale the camera matrix K based on the input image size
    cv::Mat scaled_K = K.clone();
    double scale = static_cast<double>(dim1.width) / corr_ori.width;
    scaled_K *= scale;
    scaled_K.at<double>(2, 2) = 1.0; // Ensure K[2][2] remains 1.0

    // Estimate the new camera matrix for undistortion
    cv::Mat new_K;
    cv::fisheye::estimateNewCameraMatrixForUndistortRectify(
        scaled_K, D, dim2, cv::Matx33d::eye(), new_K, balance
    );

    // Initialize the undistort rectify map
    cv::Mat map1, map2;
    cv::fisheye::initUndistortRectifyMap(
        scaled_K, D, cv::Matx33d::eye(), new_K, dim3, CV_16SC2, map1, map2
    );

    // Undistort the image using the remap function
    cv::Mat undistorted;
    cv::remap(distorted, undistorted, map1, map2, cv::INTER_LINEAR, cv::BORDER_CONSTANT);

    return undistorted;
}

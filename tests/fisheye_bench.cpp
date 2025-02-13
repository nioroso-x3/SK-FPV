
#include <thread>
#include <future>
#include "fisheye.h"



int main(int argc, char* argv[]){
 // Define the camera matrix K
  cv::Mat K = (cv::Mat_<double>(3, 3) <<
      430.8717763440679, 0.0, 621.5667829869741,
      0.0, 439.8827339778281, 362.0940179823153,
      0.0, 0.0, 1.0
  );

  // Define the distortion coefficients D
  cv::Mat D = (cv::Mat_<double>(4, 1) <<
      0.014132200676075771,
      0.0866277275894948,
      -0.12079661379707426,
      0.041586276179513534
    );

  cv::Mat input(cv::Size(1280,720),CV_8UC4);
  cv::Mat output;
  cv::Size corr_ori(1280,720);
  for (int i=0; i < 1000; ++i){
    std::future<cv::Mat> f1 = std::async(std::launch::async, undistortImage, input.clone(), K, D, corr_ori, 0.77);
    std::future<cv::Mat> f2 = std::async(std::launch::async, undistortImage, input.clone(), K, D, corr_ori, 0.77);
    output = f1.get();
    output = f2.get();
  }

  return 0;
}

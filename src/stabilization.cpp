#include "stabilization.h"

stabilizer::stabilizer(){

  downSample = 1.0f;
  zoomFactor = 1.0f;
  processVar = 0.03f;
  measVar = 2.0f;
  roiDiv = 3.0f;
  Q = cv::Mat(1, 3, CV_32F, processVar);
  R = cv::Mat(1, 3, CV_32F, measVar);
  m = cv::Mat(2,3,CV_32F); 
  count = 0;
  x = 0;
  y = 0;
  a = 0;
  dx = 0; 
  dy = 0; 
  da = 0;
  std::cout << "stabilizer initialized" << std::endl;
}

void 
stabilizer::stabilize(cv::Mat &buf,cv::Mat &st_buf){
  //if (count > 3) return;
  orig = buf.clone();
  currFrame = orig.clone();

  int res_w_orig = currFrame.cols;
  int res_h_orig = currFrame.rows;
  int res_w = (float(res_w_orig) * downSample);
  int res_h = (float(res_h_orig) * downSample);
  int top_left_x = float(res_w)/roiDiv;
  int top_left_y = float(res_h)/roiDiv;
  int roi_w = int(res_w - (float(res_w)/roiDiv)) - top_left_x;
  int roi_h = int(res_h - (float(res_h)/roiDiv)) - top_left_y;

  cv::Rect roi = cv::Rect(top_left_x,top_left_y,roi_w,roi_h);

  cv::Size frameSize = cv::Size(res_w,res_h);
  if (downSample < 1.0){
    cv::resize(orig,currFrame,frameSize);
    orig = currFrame.clone();
  }
  else{
    currFrame = orig.clone();
  }

  cv::cvtColor(currFrame, currGray, cv::COLOR_BGR2GRAY);
  cv::Mat tmp = currGray(roi).clone();
  currGray = tmp.clone();
   
  if (prevFrame.empty()){
    prevOrig = orig.clone();
    prevFrame = currFrame.clone();
    prevGray = currGray.clone();
  }
  // Optical flow parameters
  cv::Size winSize(15, 15);
  int maxLevel = 3;
  cv::TermCriteria criteria(cv::TermCriteria::EPS | cv::TermCriteria::COUNT, 10, 0.03);

  std::vector<cv::Point2f> prevPts, currPts;
  std::vector<cv::Point2f> goodPrevPts, goodCurrPts;
  std::vector<uint8_t> status;
  std::vector<float> err;
  cv::goodFeaturesToTrack(prevGray, prevPts, 400, 0.01, 30);
  if (prevPts.size()){
    cv::calcOpticalFlowPyrLK(prevGray, currGray, prevPts, currPts, status, err, winSize, maxLevel, criteria);
    for (size_t i = 0; i < status.size(); i++) {
       if (status[i] == 1) {
         goodPrevPts.push_back(prevPts[i] + cv::Point2f(int(res_w_orig / roiDiv), int(res_h_orig / roiDiv)));
         goodCurrPts.push_back(currPts[i] + cv::Point2f(int(res_w_orig / roiDiv), int(res_h_orig / roiDiv)));
      }
    }
    prevPts = goodPrevPts;
    currPts = goodCurrPts;
    if (currPts.size() && prevPts.size()){
      m = cv::estimateAffinePartial2D(prevPts, currPts);
    }
    if (m.empty()) {
      m = lastRigidTransform;
    }
    
    dx = m.at<double>(0, 2);
    dy = m.at<double>(1, 2);
    da = atan2(m.at<double>(1, 0), m.at<double>(0, 0));
  }
  else{
    dx = 0;
    dy = 0;
    da = 0;
  }
  x += dx;
  y += dy;
  a += da;

  cv::Mat Z = cv::Mat({1,3},{x,y,a});
  if (count == 0) {
    X_estimate = cv::Mat::zeros(1, 3, CV_32F);
    P_estimate = cv::Mat::ones(1, 3, CV_32F);
  } else {
    cv::Mat X_predict = X_estimate.clone();
    cv::Mat P_predict = P_estimate + Q;
    cv::Mat K = P_predict / (P_predict + R);
    X_estimate = X_predict + K.mul(Z - X_predict);
    P_estimate = (cv::Mat::ones(1, 3, CV_32F) - K).mul(P_predict);
    
  }

  float diff_x = X_estimate.at<float>(0, 0) - x;
  float diff_y = X_estimate.at<float>(0, 1) - y;
  float diff_a = X_estimate.at<float>(0, 2) - a;
  dx += diff_x;
  dy += diff_y;
  da += diff_a;
  m = cv::Mat::zeros(2,3,CV_32F);
  m.at<float>(0, 0) = cos(da);
  m.at<float>(0, 1) = -sin(da);
  m.at<float>(1, 0) = sin(da);
  m.at<float>(1, 1) = cos(da);
  m.at<float>(0, 2) = dx;
  m.at<float>(1, 2) = dy;
  cv::Mat fS, f_stabilized;
  cv::warpAffine(prevOrig, fS, m, cv::Size(res_w_orig, res_h_orig));
  cv::Size s = fS.size();
  cv::Mat T = cv::getRotationMatrix2D(cv::Point2f(s.width / 2, s.height / 2), 0, zoomFactor);
  cv::warpAffine(fS, f_stabilized, T, s); 
  
  prevOrig = orig.clone();
  prevFrame = currFrame.clone();
  prevGray = currGray.clone();
  lastRigidTransform = m.clone();
  int fwt = f_stabilized.cols * 0.1;
  int fht = f_stabilized.rows * 0.1;
  int fw = f_stabilized.cols * 0.8;
  int fh = f_stabilized.rows * 0.8;
  st_buf = f_stabilized(cv::Rect(fwt,fht,fw,fh)).clone();
  count += 1;
}


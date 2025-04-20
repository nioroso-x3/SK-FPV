#include "stabilization.h"

stabilizer::stabilizer(){

  downSample = 1.0f/2.0f;
  zoomFactor = 1.0f;
  processVar = 0.05f;
  measVar = 2.5f;
  roiDiv = 4.0f;
  Q = cv::Mat(1, 3, CV_64F, processVar);
  R = cv::Mat(1, 3, CV_64F, measVar);
  m = cv::Mat::zeros(2, 3, CV_64F);
  m.at<double>(0, 0) = 1;
  m.at<double>(1, 1) = 1;
  lastRigidTransform = m.clone(); 
  xform = m.clone();
  count = 0;
  x = 0;
  y = 0;
  a = 0;
  dx = 0; 
  dy = 0; 
  da = 0;
  std::cout << "  Stabilizer initialized" << std::endl;
}


void
stabilizer::stabilize(cv::Mat &buf, float* rx, float* ry, float* ra){
  orig = buf.clone();
  currFrame = orig.clone();

  int res_w_orig = currFrame.cols;
  int res_h_orig = currFrame.rows;
  int res_w = (double(res_w_orig) * downSample);
  int res_h = (double(res_h_orig) * downSample);
  int top_left_x = double(res_w)/roiDiv;
  int top_left_y = double(res_h)/roiDiv;
  int roi_w = int(res_w - (double(res_w)/roiDiv)) - top_left_x;
  int roi_h = int(res_h - (double(res_h)/roiDiv)) - top_left_y;
  
  cv::Rect roi = cv::Rect(top_left_x,top_left_y,roi_w,roi_h);

  cv::Size frameSize = cv::Size(res_w,res_h);
  if (downSample < 1.0){
    cv::resize(orig,currFrame,frameSize, 0, 0, cv::INTER_NEAREST);
  }
  else{
    currFrame = orig.clone();
  }

  cv::cvtColor(currFrame, currGray, cv::COLOR_BGR2GRAY);
  cv::Mat tmp = currGray(roi).clone();
  currGray = tmp.clone();
   
  if (prevFrame.empty() || prevFrame.size() != currFrame.size()){
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
    if (m.data == nullptr) {
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
    X_estimate = cv::Mat::zeros(1, 3, CV_64F);
    P_estimate = cv::Mat::ones(1, 3, CV_64F);
  } else {
    cv::Mat X_predict = X_estimate.clone();
    cv::Mat P_predict = P_estimate + Q;
    cv::Mat K = P_predict / (P_predict + R);
    X_estimate = X_predict + K.mul(Z - X_predict);
    P_estimate = (cv::Mat::ones(1, 3, CV_64F) - K).mul(P_predict);
    
  }
  double diff_x = X_estimate.at<double>(0, 0) - x;
  double diff_y = X_estimate.at<double>(0, 1) - y;
  double diff_a = X_estimate.at<double>(0, 2) - a;
  dx += diff_x;
  dy += diff_y;
  da += diff_a;
  m = cv::Mat::zeros(2,3,CV_64F);
  m.at<double>(0, 0) = cos(da);
  m.at<double>(0, 1) = -sin(da);
  m.at<double>(1, 0) = sin(da);
  m.at<double>(1, 1) = cos(da);
  m.at<double>(0, 2) = dx;
  m.at<double>(1, 2) = dy;
  *rx = (float)dx;
  *ry = (float)dy;
  *ra = (float)da;
  //clamp values and scale
  float mX = res_w_orig * 0.04;
  float mY = res_h_orig * 0.04;
  if (*rx > mX) *rx = mX;
  if (*rx < -mX) *rx = -mX;
  if (*ry > mY) *ry = mY;
  if (*ry < -mY) *ry = -mY;
  *rx /= res_w_orig;
  *ry /= res_h_orig;
  if (da > 0.0872665f) *ra = 0.087266f;
  if (da < -0.0872665f) *ra = -0.087266f;
  //clamped transform matrix
  xform = m.clone();
  xform.at<double>(0, 0) = cos(*ra);
  xform.at<double>(0, 1) = -sin(*ra);
  xform.at<double>(1, 0) = sin(*ra);
  xform.at<double>(1, 1) = cos(*ra);
  xform.at<double>(0, 2) = *rx;
  xform.at<double>(1, 2) = *ry;
  
  //store data for next iteration
  prevOrig = orig.clone();
  prevFrame = currFrame.clone();
  prevGray = currGray.clone();
  lastRigidTransform = m.clone();
  count++;
}

void
stabilizer::stabilize_i(cv::Mat &buf){
  float a,b,c;
  stabilize(buf,&a,&b,&c);
}

cv::Mat stabilizer::getFrame(){
  return orig.clone();
}

cv::Mat stabilizer::getPrevFrame(){
  return prevOrig.clone();
}

cv::Mat stabilizer::getStabFrame(){
  cv::Mat fS, f_stabilized;
  cv::warpAffine(prevOrig, fS, xform, cv::Size(prevOrig.cols, prevOrig.rows));
  cv::Size s = fS.size();
  cv::Mat T = cv::getRotationMatrix2D(cv::Point2f(s.width / 2, s.height / 2), 0, 1.0);
  cv::warpAffine(fS, f_stabilized, T, s);
  cv::Rect roi(s.width*0.05,s.height*0.05,s.width*0.9,s.height*0.9);
  return f_stabilized(roi).clone();
}

void stabilizer::setZoomFactor(double f){
  zoomFactor = f;
}
void stabilizer::setProcessVar(double f){
  processVar = f;
}
void stabilizer::setMeasVar(double f){
  measVar = f;
}
void stabilizer::setRoiDiv(double f){
  roiDiv = f;
}


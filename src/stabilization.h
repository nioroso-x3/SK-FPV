#ifndef STAB_H
#define STAB_H
#include "common.h"

class stabilizer
{
  private:
    double downSample;
    double zoomFactor;
    double processVar;
    double measVar;
    double roiDiv;
    cv::Mat Q;
    cv::Mat R;

    int count;
    double x, y, a;
    double dx, dy, da;
    cv::Mat orig;
    cv::Mat prevOrig;
    cv::Mat currFrame;
    cv::Mat currGray;
    cv::Mat prevFrame;
    cv::Mat prevGray;
    cv::Mat m;
    cv::Mat X_estimate;
    cv::Mat P_estimate;
    cv::Mat lastRigidTransform;
  //writes stablized frame based on the last two frames to the Mat reference
  public:
    stabilizer();
    void stabilize(cv::Mat &buf,cv::Mat &st_buf);

};



#endif

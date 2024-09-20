#ifndef STAB_H
#define STAB_H
#include <opencv2/opencv.hpp>
#include <vector>

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
  //writes the translation and rotation required to stabilize the video to rx, ry and ra
  public:
    stabilizer();
    void stabilize(cv::Mat &buf, float* rx, float* ry, float* ra);
    cv::Mat getFrame(void);
    cv::Mat getPrevFrame(void);
    cv::Mat getStabFrame(void);
    void setZoomFactor(double f);
    void setProcessVar(double f);
    void setMeasVar(double f);
    void setRoiDiv(double f);
};



#endif

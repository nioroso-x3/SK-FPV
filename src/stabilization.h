#ifndef STAB_H
#define STAB_H
#include "common.h"


//writes stablized frame based on the last two frames to the Mat reference
void stabilize(cv::Mat &buf, cv::Mat[] tmpbufs);



#endif

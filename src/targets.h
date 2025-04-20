#ifndef TARGETS_H
#define TARGETS_H

#include <stereokit.h>
#include <stereokit_ui.h>
#include <opencv2/opencv.hpp>
#include <zmq.hpp>
#include <msgpack.hpp>
#include <string>
#include <vector>
#include <map>

using namespace sk;

void draw_box(float   x1f, 
              float   y1f, 
              float   x2f, 
              float   y2f, 
              int     cls, 
              float   p, 
              cv::Mat &img);


void listen_zmq(const std::string              &bind_address, 
                std::map<std::string,cv::Mat>  &overlays,
                std::map<std::string,cv::Size> &vsizes);

#endif

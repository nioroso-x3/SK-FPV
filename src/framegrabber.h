#ifndef FRAMEGRABBER_H
#define FRAMEGRABBER_H
#include <stereokit.h>
#include <stereokit_ui.h>
#include <thread>
#include <future>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <string>
#include <chrono>
#include <fstream>
#include <opencv2/opencv.hpp>
#include <nlohmann/json.hpp>
#include "stabilization.h"
#include "fisheye.h"

using namespace sk;


class FrameWriter{
  private:
    std::shared_ptr<std::thread> t;
    std::string gst_pipeline;
    bool stab;
    bool playing;
    bool run;
    int type;
    cv::Mat K;
    cv::Mat D;
    cv::Size corr_ori;
    float balance;
    void stream(tex_t *vid0, tex_t *vid1);
  public:
    FrameWriter();
    ~FrameWriter();
    FrameWriter(std::string pipeline, tex_t &vid0, tex_t &vid1, bool stabilization, cv::Mat Km, cv::Mat Dm, float b, cv::Size corr_ori_r, int stype);
    void play();
    void start(std::string pipeline, tex_t &vid0, tex_t &vid1, bool stabilization, cv::Mat Km, cv::Mat Dm, float b, cv::Size corr_ori_r, int stype);
    void stop();
    void terminate();
    void toggleStab();
};



class VideoContainer{
  public:
    VideoContainer();
    VideoContainer(std::string jsonpath);
    VideoContainer(nlohmann::json json);
    std::mutex                        mtx;
    std::map<std::string,mesh_t>      mesh_ts;
    std::map<std::string,material_t>  material_ts;
    std::map<std::string,pose_t>      pose_ts;
    std::map<std::string,tex_t>       tex_ts;
    std::map<std::string,FrameWriter> frame_caps;

    void add_surface(std::string name, float ratio, float scale, pose_t pose, bool transparent);
    void del_surface(std::string name);
    std::vector<std::string> list_names();
    void load_file(std::string jsonpath);
    void load_json(nlohmann::json json);
   
};





#endif

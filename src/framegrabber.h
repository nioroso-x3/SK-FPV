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
#include <atomic>
#include <opencv2/opencv.hpp>
#include <nlohmann/json.hpp>
#include "stabilization.h"
#include "fisheye.h"

using namespace sk;

class FrameWriter{
  private:
    std::shared_ptr<std::thread> t;
    std::shared_ptr<std::thread> monitor_thread;
    std::string name;
    std::string oname;
    std::string gst_pipeline;
    bool stab;
    bool playing;
    bool run;
    std::atomic<bool> monitor_run{true};
    int type;
    cv::Mat K;
    cv::Mat D;
    cv::Size corr_ori;
    float balance;
    std::chrono::steady_clock::time_point last_frame_time;
    std::atomic<float> video_brightness{1.0f};
    void stream(tex_t *vid0,
                tex_t *vid1,
                tex_t *ov0,
                tex_t *ov1,
                std::map<std::string,cv::Mat> *overlays,
                std::map<std::string,cv::Size> *vsizes);
    void start_monitor();
    void monitor_video_timeout();
  public:
    FrameWriter();
    ~FrameWriter();
    FrameWriter(std::string name, 
                std::string gst_pipeline, 
                std::map<std::string,cv::Mat> &overlays, 
                std::map<std::string,cv::Size> &vsizes, 
                tex_t &vid0, 
                tex_t &vid1, 
                tex_t &ov0, 
                tex_t &ov1, 
                bool stab, 
                cv::Mat K, 
                cv::Mat D, 
                float balance, 
                cv::Size corr_ori, 
                int type);
    void start(std::string name, 
               std::string gst_pipeline, 
               std::map<std::string,cv::Mat> &overlays, 
               std::map<std::string,cv::Size> &vsizes, 
               tex_t &vid0, 
               tex_t &vid1, 
               tex_t &ov0, 
               tex_t &ov1, 
               bool stab, 
               cv::Mat K, 
               cv::Mat D, 
               float balance, 
               cv::Size corr_ori, 
               int type);
    void play();
    void stop();
    void terminate();
    void toggleStab();
    float get_brightness() const;
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
    std::map<std::string,cv::Mat>     overlays;
    std::map<std::string,cv::Size>    vsizes;
    void add_surface(std::string name, float ratio, float scale, pose_t pose, bool transparent);
    void add_video_shader(std::string name, const cv::Mat K, const cv::Mat D, const cv::Size imageSize, double balance);
    void del_surface(std::string name);
    std::vector<std::string> list_names();
    void load_file(std::string jsonpath);
    void load_json(nlohmann::json json);
   
};





#endif

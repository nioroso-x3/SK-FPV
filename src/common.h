#ifndef COMMON_H
#define COMMON_H

#include <stereokit.h>
#include <stereokit_ui.h>
#include <opencv2/opencv.hpp>
#include <iostream> 
#include <thread>
#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/action/action.h>
#include <mavsdk/plugins/telemetry/telemetry.h>
#include <mavsdk/plugins/mavlink_passthrough/mavlink_passthrough.h>
#include <chrono>
#include <cmath>

#define HUD_COLOR cv::Scalar(0,255,0,200)
#define HUD_COLOR_CLR cv::Scalar(0,0,0,0)
using namespace mavsdk;
using namespace sk;

 //vector with the flightmodes
extern std::string fmodes[];
extern std::string fmodes_PX4[];

//pose_t controls location, X (left/right), Y(up/down), Z(front/back). The second argument is a quartenion, you can search for calculators if you want to change the screen angle and rotation.

//Globals so all threads can access them

//first screen
extern mesh_t     plane_mesh;
extern material_t plane_mat;
extern pose_t     plane_pose;
extern tex_t vid0;

//second screen
extern mesh_t     plane1_mesh;
extern material_t plane1_mat;
extern pose_t     plane1_pose;
extern tex_t vid1;

//ground stereo cameras
extern mesh_t     plane2_mesh;
extern material_t plane2_mat;
extern pose_t     plane2_pose;
extern tex_t vid2;
extern bool gnd_cam_color;

extern mesh_t     plane3_mesh;
extern material_t plane3_mat;
extern pose_t     plane3_pose;
extern tex_t vid3;

//left window
extern mesh_t     plane4_mesh;
extern material_t plane4_mat;
extern pose_t     plane4_pose;
extern tex_t vid4;

//right window
extern mesh_t     plane5_mesh;
extern material_t plane5_mat;
extern pose_t     plane5_pose;
extern tex_t vid5;

//Map
extern mesh_t     plane6_mesh;
extern material_t plane6_mat;
extern pose_t     plane6_pose;
extern tex_t vid6;
extern double map_zoom;

//cubemap for skybox
extern tex_t sky;

//HUD
extern mesh_t     hud_mesh;
extern material_t hud_mat;
extern pose_t     hud_pose;
extern tex_t      hud_tex;

//video and hud scales
extern float p1s;
extern float p2s;
extern float p3s;
extern float pWs;
extern float hud_s;


extern bool run_stab;
extern bool gnd_cam;


//renderer frame counter
extern uint64_t cnt;

//vehicle position
//attributes are latitude_deg, longitude_deg absolute_altitude_m and relative_altitude_m
extern Telemetry::Position vh_pos;

//vehicle attitude
//attributes are roll_deg, pitch_deg, and yaw_deg
// positive is bank clockwise, nose up and yaw right
extern Telemetry::EulerAngle vh_att;

//vehicle batteries 0 and 1
//attributes are id, voltage_v, current_battery_a
extern Telemetry::Battery vh_bat0;
extern Telemetry::Battery vh_bat1;

//vehicle airspeed_m_s, throttle_percentage and climb_rate_m_s
extern Telemetry::FixedwingMetrics vh_fwing;

//vehicle gps info, num_satellites and fix_type
extern Telemetry::GpsInfo vh_gpsi;

//vehicle raw gps info, velocity_m_s is the ground speed
extern Telemetry::RawGps vh_gpsr;

//vehicle status messages
//should hold the last status
extern int status_counter;
extern Telemetry::StatusText vh_st_text;

//rc signal strenght
extern Telemetry::RcStatus vh_rc;

//flight mode
extern std::string vh_fmode;

//home position
extern Telemetry::Position home_pos;


//wfb mavlink stream statistics
extern int8_t wfb_rssi; //WFB rssi
extern uint16_t wfb_errors;
extern uint16_t wfb_fec_fixed;
extern int8_t wfb_flags;

//distance sensors, for now only the landing sensor is supported
extern Telemetry::DistanceSensor vh_rngfnd;

//gps time
extern uint64_t gps_time;

float get_heading(float,float,float,float);
float get_distance(float,float,float,float);

#endif

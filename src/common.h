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
#include <chrono>
#include <cmath>

#define HUD_COLOR cv::Scalar(0,255,0,160)
#define HUD_COLOR_CLR cv::Scalar(0,0,0,0)
using namespace mavsdk;
using namespace sk;
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

//HUD
extern mesh_t     hud_mesh;
extern material_t hud_mat;
extern pose_t     hud_pose;
extern tex_t      hud_tex;



//Double buffers for the screens
extern cv::Mat buffer0[2];
extern cv::Mat buffer1[2];
extern cv::Mat buffer2[2];

//Current buffer being used
extern char cur_buffer0;
extern char cur_buffer1;
extern char cur_buffer2;

extern uint64_t cnt;

//vehicle position
//attributes are latitude_deg, longitude_deg absolute_altitude_m and relative_altitude_m
extern Telemetry::Position vh_pos;

//vehicle attitude
//attributes are roll_deg, pitch_deg, and yaw_deg
// positive is bank right, nose up and rotating right
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

#endif

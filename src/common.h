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
#include <map>
#include "framegrabber.h"

using namespace mavsdk;
using namespace sk;

 //vector with the flightmodes
extern std::string fmodes[];
extern std::string fmodes_PX4[];

//pose_t controls location, X (left/right), Y(up/down), Z(front/back). The second argument is a quartenion, you can search for calculators if you want to change the screen angle and rotation.

//Globals so all threads can access them
//

extern VideoContainer vsurfaces;



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

//main camera selector
extern int cam_selector;

//ground station rx stream stats
extern std::map<std::string,std::vector<int>> wfb_rx; 

float get_heading(float,float,float,float);
float get_distance(float,float,float,float);


#endif

#include "common.h"

using namespace sk;
using namespace mavsdk;


 //vector with the flightmodes
std::string fmodes_PX4[] ={
    "Unknown",
    "FBWA",
    "Autotune",
    "Guided",
    "Ready",
    "Takeoff",
    "Hold",
    "Mission",
    "RTL",
    "Land",
    "Offboard",
    "FollowMe",
    "Manual",
    "Altctl",
    "Posctl",
    "Acro",
    "Rattitude",
    "Stabilized"};


std::string fmodes[] ={
"MANUAL",
"CIRCLE",
"STABILIZE",
"TRAINING",
"ACRO",
"FBWA",
"FBWB",
"CRUISE",
"AUTOTUNE",
"UNKNOWN_9",
"Auto",
"RTL",
"Loiter",
"TAKEOFF",
"AVOID_ADSB",
"Guided",
"QSTABILIZE",
"QHOVER",
"QLOITER",
"QLAND",
"QRTL",
"QAUTOTUNE",
"QACRO",
"THERMAL",
"LOITERQLAND"};


VideoContainer vsurfaces;

//cubemap for skybox
tex_t sky;

//HUD
mesh_t     hud_mesh;
material_t hud_mat;
pose_t     hud_pose;
tex_t      hud_tex;

//map screen
mesh_t     plane6_mesh;
material_t plane6_mat;
//pose_t     plane6_pose = {{-1.5f,-1.1f,-1.1f}, {-0.258819,0.0,0.0,0.9659258}};
//pose_t     plane6_pose = {{-1.7f,1.0f,-1.99f}, {0,0,0,1}};
pose_t     plane6_pose = {{0,0,-0.01}, {0,0,0,1}};
tex_t      vid6;
double     map_zoom = 16;



float p1s = 2.0f;
float p2s = 1.0f;
float p3s = 1.0f;
float pWs = 2.2f;
float hud_s = 0.5f;

uint64_t cnt = 0;


//vehicle position
//attributes are latitude_deg, longitude_deg absolute_altitude_m and relative_altitude_m
Telemetry::Position vh_pos;

//vehicle attitude
//attributes are roll_deg, pitch_deg, and yaw_deg
// positive is bank right, nose up and rotating right
Telemetry::EulerAngle vh_att;

//vehicle batteries 0 and 1
//attributes are id, voltage_v, current_battery_a
Telemetry::Battery vh_bat0;
Telemetry::Battery vh_bat1;

//vehicle airspeed_m_s, throttle_percentage and climb_rate_m_s
Telemetry::FixedwingMetrics vh_fwing;

//vehicle gps info, num_satellites and fix_type
Telemetry::GpsInfo vh_gpsi;

//vehicle raw gps info, velocity_m_s is the ground speed
Telemetry::RawGps vh_gpsr;


//vehicle status messages
//should hold the last status
int status_counter = 0;
Telemetry::StatusText vh_st_text;

//rc signal strenght
Telemetry::RcStatus vh_rc;

//flight mode
std::string vh_fmode = "UNKNOWN";

//home position
Telemetry::Position home_pos;

//lte
uint8_t lte_stat = 0;
uint8_t lte_f_reason = 0;
uint8_t lte_type = 0;
uint8_t lte_qual = 0;

//wfb mavlink
int8_t wfb_rssi = 0;
uint16_t wfb_errors = 0;
uint16_t wfb_fec_fixed = 0;
int8_t wfb_flags = 0;

//rangefinder
Telemetry::DistanceSensor vh_rngfnd;

//unused
std::map<std::string,std::vector<int>> wfb_rx; 

//drone time
uint64_t gps_time = 0;

//aoa and ssa
float aoa = 0.0f;
float ssa = 0.0f;

int cam_selector = 0;

//hud mode
bool hud_follow_head = true;

//rc mode
bool rc_override = false;


float get_heading(float lon1, float lat1, float lon2, float lat2) {
    lon1 *= M_PI/180.0;
    lon2 *= M_PI/180.0;
    lat1 *= M_PI/180.0;
    lat2 *= M_PI/180.0;
    float y = sin(lon2 - lon1) * cos(lat2);
    float x = (cos(lat1) * sin(lat2)) - ( sin(lat1) * cos(lat2) * cos(lon2 - lon1));
    float o = atan2(y,x);
    return fmodf(o*180/M_PI + 360,360);
}

float get_distance(float lon1, float lat1, float lon2, float lat2) {
    const float R = 6371000.0; // Earth's radius in meters
    float dLat = (lat2 - lat1) * M_PI / 180.0;
    float dLon = (lon2 - lon1) * M_PI / 180.0;

    float a = sin(dLat / 2) * sin(dLat / 2) +
              cos(lat1 * M_PI / 180.0) * cos(lat2 * M_PI / 180.0) *
              sin(dLon / 2) * sin(dLon / 2);
    float c = 2 * atan2(sqrt(a), sqrt(1 - a));

    return R * c; // Distance in meters
}


//helpers for camera surface recentering
double quat2yaw(const quat& q){
    float fx =  2.0f * (q.x*q.z + q.w*q.y);
    float fz =  1.0f - 2.0f * (q.x*q.x + q.y*q.y);
    return std::atan2(fx, fz);
}

vec3 rotate(const quat& q, const vec3& v){
    const float tx = 2.0 * (q.y * v.z - q.z * v.y);
    const float ty = 2.0 * (q.z * v.x - q.x * v.z);
    const float tz = 2.0 * (q.x * v.y - q.y * v.x);

    return {
        v.x + q.w * tx + (q.y * tz - q.z * ty),
        v.y + q.w * ty + (q.z * tx - q.x * tz),
        v.z + q.w * tz + (q.x * ty - q.y * tx)
    };
}

//stores initial surface poses
std::map<std::string,pose_t> ori_poses;
void recenter_cameras() {
    const vec3 h_at = input_head()->position;
    const quat h_ori = input_head()->orientation;
    const float yaw = quat2yaw(h_ori);
    const quat yaw_q = {0.0f, std::sin(yaw*0.5f), 0.0f, std::cos(yaw*0.5f)};
    //std::cout << "Head pos: " << h_at.x << " " << h_at.y << " " << h_at.z << std::endl;
    //std::cout << "Head ori: " << h_ori.x << " " << h_ori.y << " " << h_ori.z << " " << h_ori.w << std::endl;
    //std::cout << "Head yaw: " << yaw << std::endl;
    //std::cout << "Head q_y: " << yaw_q.x << " " << yaw_q.y << " " << yaw_q.z << " " << yaw_q.w << std::endl;
    if (ori_poses.size() == 0){
        for(std::string name : vsurfaces.list_names()){
            ori_poses[name] = vsurfaces.pose_ts[name];
        }
    }
    for(std::string name : vsurfaces.list_names()){
        pose_t    pose = ori_poses[name];
        vec3 at = pose.position;
        quat ori = pose.orientation;
        //std::cout << name << " pos: " << at.x << " " << at.y << " " << at.z << std::endl;
        //std::cout << name << " ori: " << ori.x << " " << ori.y << " " << ori.z << " " << ori.w << std::endl;
        vec3 v = at - h_at;
        vec3 vRot = rotate(yaw_q, v);
        
        at = h_at + vRot;
        ori = ori * yaw_q;
        //std::cout << name << " new pos: " << at.x << " " << at.y << " " << at.z << std::endl;
        //std::cout << name << " new ori: " << ori.x << " " << ori.y << " " << ori.z << " " << ori.w << std::endl;
        vsurfaces.pose_ts[name].position = at;
        vsurfaces.pose_ts[name].orientation = ori;
        
    }

}

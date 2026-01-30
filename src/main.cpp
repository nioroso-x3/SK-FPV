#include "common.h"
#include "hud.h"
#include "mavlink_setup.h"
#include "stabilization.h"
#include "mapping.h"
#include "wfb_stats.h"
#include "targets.h"
#include "joystick_config.h"
#include "joystick_openxr.h"
#include <nlohmann/json.hpp>
#include <fstream>

using namespace sk;
using namespace mavsdk;

// Global OpenXR joystick instance
OpenXRJoystickInput* g_openxr_joystick = nullptr;

// Video streaming configuration
struct VideoStreamConfig {
    bool enabled = false;
    int width = 1280;
    int height = 720;
    int fov = 54;
    int capture_interval = 3;
    std::string gstreamer_pipeline;
};

VideoStreamConfig video_config;

//video output
cv::VideoWriter output;
//callback needed for the video rendering function
void scr_callback(color32* buffer, int w, int h, void *ctx){
    cv::Mat out(w, h, CV_8UC4, buffer);
    cv::cvtColor(out,out,cv::COLOR_RGBA2BGR);
    if (output.isOpened())
    output << out;
}

std::string getUnixTimestampAsString() {
    auto now = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
    std::string timestampString = std::to_string(currentTime);
    return timestampString;
}

bool loadVideoStreamConfig(const std::string& config_path) {
    try {
        std::ifstream file(config_path);
        if (!file.is_open()) {
            std::cout << "Video stream config file not found: " << config_path << std::endl;
            return false;
        }

        nlohmann::json j;
        file >> j;

        if (j.contains("video_streaming")) {
            auto vs = j["video_streaming"];

            video_config.enabled = vs.value("enabled", false);

            if (vs.contains("resolution")) {
                video_config.width = vs["resolution"].value("width", 1280);
                video_config.height = vs["resolution"].value("height", 720);
            }

            video_config.fov = vs.value("fov", 54);
            video_config.capture_interval = vs.value("capture_interval", 3);

            std::string pipeline = vs.value("gstreamer_pipeline", "");
            if (!pipeline.empty()) {
                // Replace {timestamp} placeholder with actual timestamp
                size_t pos = pipeline.find("{timestamp}");
                if (pos != std::string::npos) {
                    pipeline.replace(pos, 11, getUnixTimestampAsString());
                }
                video_config.gstreamer_pipeline = pipeline;
            }
        }

        std::cout << "Video streaming config loaded:" << std::endl;
        std::cout << "  Enabled: " << (video_config.enabled ? "YES" : "NO") << std::endl;
        std::cout << "  Resolution: " << video_config.width << "x" << video_config.height << std::endl;
        std::cout << "  FOV: " << video_config.fov << "°" << std::endl;
        std::cout << "  Capture interval: " << video_config.capture_interval << " frames" << std::endl;

        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error loading video stream config: " << e.what() << std::endl;
        return false;
    }
}

void mavlink_thread()
{
  start_mavlink_thread();
  std::cerr << "Mavlink thread exited" << std::endl;
}

void draw_hud()
{
  std::cerr << "Starting HUD thread" << std::endl;
  draw_cairo_hud();
  std::cerr << "HUD thread exited" << std::endl;
}

void map_thread(){
  std::cerr << "Starting Map thread" << std::endl;
  cv::Rect roi(0,0,MAP_W,MAP_H);
  cv::Mat cur_loc;
  cv::Mat mask(MAP_W,MAP_H,CV_8UC4,cv::Scalar(0,0,0,255));
  int cx = MAP_W/2 - 1;
  int cy = MAP_H/2 - 1;
  //draw triangle representing vehicle
  cv::Point p1( cx   , cy-10);
  cv::Point p2( cx-10, cy+15);
  cv::Point p3( cx+10, cy+15);
  cv::Scalar color(128,255,128,255);
  //draw circular transparent mask
  cv::circle(mask,cv::Point(cx,cy), cx , cv::Scalar(0,0,0,0), -1, cv::LINE_AA); 
  //set to your API key and style JSON
  maplibre maps(std::string(""),std::string("file://./style.json"));
  //load the circle and home markers
  maps.add_icon(std::string("circle"),std::string("circle.png"));
  maps.add_icon(std::string("home"),std::string("home.png"));
  while(true){
    cv::Mat output(1440,1920,CV_8UC4,cv::Scalar(0,0,0,0));
    //set home in the map
    int home_set = maps.set_home(home_pos.latitude_deg,home_pos.longitude_deg);
    if (maps.get_map(&cur_loc,vh_pos.latitude_deg,vh_pos.longitude_deg,vh_att.yaw_deg,map_zoom) == 0){
      cv::line(cur_loc,p1,p2,color,2,cv::LINE_AA);
      cv::line(cur_loc,p1,p3,color,2,cv::LINE_AA);
      cv::line(cur_loc,p2,p3,color,2,cv::LINE_AA);
      cur_loc = cur_loc - mask;
      cur_loc.copyTo(output(roi));
      tex_set_colors(vid6,output.cols,output.rows,(void*)output.datastart);
    }
    //mavlink gps stream at most at 10Hz, so 20 fps is enough.
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  std::cerr << "Map thread exited" << std::endl;
}

void wfbgs_thread(){
  wfb_stats wfb;
  std::cout << "WFB rx stats thread started" << std::endl;
  while(true){

    int openres =  wfb.open("127.0.0.1",8003);
    if (openres != 0){
      std::cerr << "WFB: Waiting 10 seconds" << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(10000));
      continue; 
    }
    wfb.start(wfb_rx);
  }
  std::cout << "WFB rx stats thread finished" << std::endl;
}

void zmq_thread(){
  std::string uri("tcp://0.0.0.0:6600");
  listen_zmq(uri, vsurfaces.overlays, vsurfaces.vsizes);
}

void joystick_thread(){
  std::cout << "Starting joystick thread" << std::endl;

  // Register joystick functions
  JoystickConfig::registerFunction("hud_toggle", [](){
    hud_follow_head = !hud_follow_head;
    std::cout << "HUD follow head: " << (hud_follow_head ? "ON" : "OFF") << std::endl;
  });

  JoystickConfig::registerFunction("cam_switch", [](){
    cam_selector = (cam_selector + 1) % 4; // Cycle through 4 cameras
    std::cout << "Camera switched to: " << cam_selector << std::endl;
  });
  JoystickConfig::registerFunction("rc_toggle", [](){
    if (g_joystick_config) {
      g_joystick_config->toggleRcOverride();
    }
  });

  JoystickConfig::registerFunction("recenter_cameras", [](){
    recenter_cameras();
  });

  // Flight mode functions
  JoystickConfig::registerFunction("mode_manual", set_flight_mode_manual);
  JoystickConfig::registerFunction("mode_stabilize", set_flight_mode_stabilize);
  JoystickConfig::registerFunction("mode_fbwa", set_flight_mode_fbwa);
  JoystickConfig::registerFunction("mode_cruise", set_flight_mode_cruise);
  JoystickConfig::registerFunction("mode_auto", set_flight_mode_auto);
  JoystickConfig::registerFunction("mode_rtl", set_flight_mode_rtl);
  JoystickConfig::registerFunction("mode_loiter", set_flight_mode_loiter);
  JoystickConfig::registerFunction("mode_guided", set_flight_mode_guided);

  // Determine joystick type from configuration
  JoystickType joystick_type = g_joystick_config ? g_joystick_config->getSettings().type : JoystickType::LIBEVDEV;

  if (joystick_type == JoystickType::OPENXR) {
    std::cout << "Using OpenXR/VR controller input" << std::endl;

    // Initialize OpenXR joystick
    g_openxr_joystick = new OpenXRJoystickInput();

    if (!g_openxr_joystick->initialize()) {
      std::cerr << "Failed to initialize OpenXR joystick" << std::endl;
      delete g_openxr_joystick;
      g_openxr_joystick = nullptr;
      return;
    }

    // Start OpenXR joystick input processing
    g_openxr_joystick->start();

    // Keep thread alive while joystick is running
    while (g_openxr_joystick && g_openxr_joystick->getState().connected) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  } else {
    std::cout << "Using libevdev external joystick input" << std::endl;

    // Initialize libevdev joystick (original implementation)
    g_joystick = new JoystickInput();

    if (!g_joystick->initialize()) {
      std::cerr << "Failed to initialize joystick" << std::endl;
      delete g_joystick;
      g_joystick = nullptr;
      return;
    }

    // Start joystick input processing
    g_joystick->start();

    // Keep thread alive while joystick is running
    while (g_joystick && g_joystick->getState().connected) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }

  std::cout << "Joystick thread exited" << std::endl;
}

int main(int argc, char* argv[]) {

  // Initialize global variables
  hud_follow_head = true; // HUD follows head by default

  // Initialize joystick configuration
  g_joystick_config = new JoystickConfig();
  if (!g_joystick_config->loadFromFile("joystick_config.json")) {
    std::cout << "Using default joystick configuration" << std::endl;
  }

  // Sync global RC override state with config
  rc_override = g_joystick_config->isRcOverrideEnabled();

  // Load video streaming configuration
  loadVideoStreamConfig("video_stream_config.json");

  //initialize stereokit
  sk_settings_t settings = {};
	
  settings.app_name           = "SK_FPV";
	settings.assets_folder      = "";
	settings.display_preference = display_mode_mixedreality;
	
  if (!sk_init(settings))
		return 1;
  
  //setup map screen, 4:3 ratio
  plane6_mesh = mesh_gen_plane({1.333f*0.78f,0.78f},{0,0,1},{0,1,0});
  plane6_mat = material_copy_id(default_id_material_unlit);
  vid6 = tex_create(tex_type_image_nomips,tex_format_rgba32);
  tex_set_address(vid6, tex_address_clamp);

  //setup hud, 3:2 ratio
  hud_mesh = mesh_gen_plane({1.5f*hud_s,hud_s},{0,0,1},{0,1,0});
  hud_mat = material_copy_id(default_id_material_unlit);
  hud_tex = tex_create(tex_type_image_nomips,tex_format_rgba32);
  tex_set_address(hud_tex, tex_address_clamp);

  
  //configure textures for map
  material_set_texture(plane6_mat,"diffuse",vid6);
  material_set_transparency(plane6_mat,transparency_blend);
  
  //configure shader and textures for hud
  shader_t hud_shader = shader_create_file("shadow.hlsl");
  material_set_shader(hud_mat,hud_shader);
  shader_release(hud_shader);
  material_set_texture(hud_mat,"diffuse",hud_tex);
  material_set_transparency(hud_mat,transparency_blend);

  std::cout << "Starting threads" << std::endl;
  std::thread t4(&draw_hud);
  std::thread t5(&mavlink_thread);
  std::thread t6(&map_thread);
  //std::thread t7(&wfbgs_thread);
  std::thread t8(&zmq_thread);
  std::thread t9(&joystick_thread);
  std::thread t10(&start_rc_override_thread);

  //hide hands, we are not using them right now.
  for (size_t h = 0; h < handed_max; h++) {
      input_hand_visible((handed_)h,false);
  }
  //disable skybox 
  render_enable_skytex(false);
  render_set_cam_root(matrix_t({0,0,1.6}));

  // Initialize video streaming if enabled
  if (video_config.enabled && !video_config.gstreamer_pipeline.empty()) {
    bool output_video = output.open(video_config.gstreamer_pipeline,
                                   cv::CAP_GSTREAMER, 0, 30,
                                   cv::Size(video_config.width, video_config.height), true);
    if (output_video) {
      std::cout << "Video streaming started: " << video_config.width << "x" << video_config.height
                << " FOV:" << video_config.fov << "°" << std::endl;
    } else {
      std::cout << "Video streaming failed to start" << std::endl;
    }
  } else {
    std::cout << "Video streaming disabled" << std::endl;
  }
  //load video surfaces
  vsurfaces.load_file("./cam.json");
  //display video surfaces names
  for(std::string name : vsurfaces.list_names()) std::cout << "Stream name:" << " " <<name << "\n";
  
  std::cout << "Starting main loop" << std::endl;
  sk_run([]() {
    //videocontainer planes
      for(std::string name : vsurfaces.list_names()){
        mesh_t    &mesh = vsurfaces.mesh_ts[name];
        material_t &mat = vsurfaces.material_ts[name];
        pose_t    &pose = vsurfaces.pose_ts[name];

        // Update darken_factor based on video frame timeout
        if(vsurfaces.frame_caps.find(name) != vsurfaces.frame_caps.end()) {
          float brightness = vsurfaces.frame_caps[name].get_brightness();
          material_set_param(mat, "darken_factor", material_param_float, &brightness);
        }
        else{
          std::string oname(name.begin(), name.end() - 2);
          float brightness = vsurfaces.frame_caps[oname].get_brightness();
          material_set_param(mat, "darken_factor", material_param_float, &brightness);

        }

        ui_handle_begin(name.c_str(), pose, mesh_get_bounds(mesh), false);
        render_add_mesh(mesh, mat, matrix_identity);
        ui_handle_end();
      }
      //HUD plane
      //setup hud position and orientation based on toggle state
      vec3 h_at, m_at;
      quat h_ori, m_ori;
      static vec3 fixed_hud_pos = {0, 0, 1.0f}; // Fixed position when not following head
      static vec3 fixed_map_pos = {0, 0, 1.05f}; // Fixed position when not following head
      static quat fixed_hud_ori = quat_identity; // Fixed orientation when not following head
      static quat fixed_map_ori = quat_identity; // Fixed orientation when not following head
      static bool fixed_pos_set = false;

      if (hud_follow_head) {
        // HUD follows head - 1 unit in front of camera, facing it
        h_at = input_head()->position + input_head()->orientation * vec3_forward * 1.0f;
        m_at = input_head()->position + input_head()->orientation * vec3_forward * 1.05f;
        h_ori = quat_lookat(input_head()->position,h_at);
        m_ori = quat_lookat(input_head()->position,m_at);
        fixed_pos_set = false; // Reset flag when following head
      } else {
        // HUD in fixed position - capture initial position when first toggled
        if (!fixed_pos_set) {
          fixed_hud_pos = input_head()->position + input_head()->orientation * vec3_forward * 1.0f;
          fixed_map_pos = input_head()->position + input_head()->orientation * vec3_forward * 1.05f;
          fixed_hud_ori = quat_lookat(input_head()->position, fixed_hud_pos);
          fixed_map_ori = quat_lookat(input_head()->position, fixed_map_pos);
          fixed_pos_set = true;
        }
        h_at = fixed_hud_pos;
        m_at = fixed_map_pos;
        h_ori = fixed_hud_ori;
        m_ori = fixed_map_ori;
      }

     
      //hud
      ui_handle_begin("HUD", hud_pose, mesh_get_bounds(hud_mesh), false);
      render_add_mesh(hud_mesh, hud_mat, matrix_trs(h_at,h_ori,vec3_one));
      ui_handle_end();
      //map
      ui_handle_begin("Plane6", plane6_pose, mesh_get_bounds(plane6_mesh), false);
      render_add_mesh(plane6_mesh, plane6_mat, matrix_trs(m_at,m_ori,vec3_one));
      ui_handle_end();
        
        // Configurable video streaming capture
        if (video_config.enabled && output.isOpened() && (cnt % video_config.capture_interval == 0)) {
          render_screenshot_capture(&scr_callback, *input_head(),
                                  video_config.width, video_config.height,
                                  video_config.fov, tex_format_rgba32, NULL);
        } 
        
        //print some debug stuff if needed
    if(cnt % 100 == 0){
            //std::cout << hud_pose.orientation.x << " " << hud_pose.orientation.y << " " << hud_pose.orientation.z << " " << hud_pose.orientation.w << std::endl << std::endl;
            //std::cout << hud_pose.position.x << " " << hud_pose.position.y << " " << hud_pose.position.z << std::endl << std::endl;
            //std::cout << vh_att.roll_deg << " " << vh_att.pitch_deg << std::endl;
            //std::cout << ori.x << " " << ori.y << " " << ori.z << " "<< ori.w << std::endl;
    }    
    cnt++;
  });
  return 0;
}

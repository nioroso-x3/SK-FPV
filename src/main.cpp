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
using namespace sk;
using namespace mavsdk;

//pose_t controls location, X (left/right), Y(up/down), Z(front/back). The second argument is a quartenion, you can search for calculators if you want to change the screen angle and rotation.

//Globals so all threads can access them

//first screen
mesh_t     plane_mesh;
material_t plane_mat;
pose_t     plane_pose = {{0,0.12,-1.5f}, {0,0,0,1}};
tex_t vid0;

//second screen
mesh_t     plane1_mesh;
material_t plane1_mat;
pose_t     plane1_pose = {{0,-1.0f,-1.2f}, {-0.258819,0,0,0.9659258}};
tex_t vid1;

//HUD
mesh_t     hud_mesh;
material_t hud_mat;
pose_t     hud_pose;
tex_t      hud_tex;



//Double buffers for the screens
cv::Mat buffer0[2];
cv::Mat buffer1[2];
cv::Mat buffer2[2];

//HUD double buffer
cv::Mat hud_buf[2];

//Current buffer being used
char cur_buffer0 = 0;
char cur_buffer1 = 0;
char cur_buffer2 = 0;
char cur_hud_buf = 0;

uint64_t cnt = 0;


cv::VideoCapture cap0;
cv::VideoCapture cap1;
cv::VideoCapture cap2;

model_t test;

//vehicle position
//attributes are latitude_deg, longitude_deg absolute_altitude_m and relative_altitude_m
Telemetry::Position vh_pos;

//vehicle attitude
//attributes are roll_deg, pitch_deg, and yaw_deg
// positive is bank right, nose up and rotating right
Telemetry::EulerAngle vh_att;

//vehicle batteries 0 and 1
//attributes are id, voltage_v, current_battery_a
Telemetry::Battery vh_bat;


//should make this only one function but whatever
void frame_grabber0()
{
  while(true){
    cap0.read(buffer0[!cur_buffer0]);
    cur_buffer0 = !cur_buffer0;
  }
}

void frame_grabber1()
{
  while(true){
    cap1.read(buffer1[!cur_buffer1]);
    cur_buffer1 = !cur_buffer1;
  }
}

void draw_hud()
{
  hud_buf[0] = cv::Mat(1024,1024,CV_8UC4,cv::Scalar(0,0,0,0));
  hud_buf[1] = cv::Mat(1024,1024,CV_8UC4,cv::Scalar(0,0,0,0));
  while(true){
      cv::Mat& cur_frame = hud_buf[!cur_hud_buf];
      //clear buffer
      cur_frame.setTo(cv::Scalar(0,0,0,0));
      //copy current values
      Telemetry::Position c_pos = vh_pos;
      Telemetry::EulerAngle c_att = vh_att;
      Telemetry::Battery c_bat = vh_bat;

      
      //draw roll
      float r = c_att.roll_deg * M_PI /180.0;

      cv::Point center(512,512);
      cv::Point right;
      cv::Point left;
      right.x = int(center.x + 256 * cos(r) );
      right.y = int(center.y + 256 * sin(r) );
      left.x=(512 - (right.x - 512));
      left.y=(512 - (right.y - 512));
      cv::line(cur_frame,left,right,cv::Scalar(0,255,0,160),2,cv::LINE_AA);
      
      //draw pitch
      float p = c_att.pitch_deg * M_PI / 180.0;



      //flip buffers
      cur_hud_buf = !cur_hud_buf;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

   }


}



model_t main_screen;







int main(void) {
//mavlink globals

  Mavsdk mavsdk{Mavsdk::Configuration{Mavsdk::ComponentType::GroundStation}};
  ConnectionResult connection_result = mavsdk.add_any_connection("udp://0.0.0.0:14540");

   if (connection_result != ConnectionResult::Success) {
       std::cerr << "Connection failed: " << connection_result << '\n';
       return 1;
   }

   auto system = mavsdk.first_autopilot(3.0);
   while(!system) {
       std::cerr << "Connection timeout on udp://0.0.0.0:14551, retrying " << std::endl;
       system = mavsdk.first_autopilot(3.0);
       std::this_thread::sleep_for(std::chrono::seconds(1));
   }


   // Instantiate plugins.
   auto telemetry = mavsdk::Telemetry{system.value()};

   telemetry.subscribe_position([](Telemetry::Position position) {
      vh_pos = position;
   });
   telemetry.subscribe_attitude_euler([](Telemetry::EulerAngle att){
      vh_att = att;
   });
   telemetry.subscribe_battery([](Telemetry::Battery bat){
      vh_bat = bat;
   });


  //initialize stereokit
  sk_settings_t settings = {};
	
  settings.app_name           = "SKNativeTemplate";
	settings.assets_folder      = "Assets";
	settings.display_preference = display_mode_mixedreality;
	
  if (!sk_init(settings))
		return 1;

  //setup first screen, we generate a plane with 16:9 ratio
  plane_mesh = mesh_gen_plane({3,1.6875},{0,0,1},{0,1,0});
  plane_mat = material_copy_id(default_id_material_unlit);
  vid0 = tex_create(tex_type_image_nomips,tex_format_rgba32);
  tex_set_address(vid0, tex_address_clamp);

  
  //setup second screen, we generate a plane with a 4:3 ratio
  plane1_mesh = mesh_gen_plane({1.25f,1},{0,0,1},{0,1,0});
  plane1_mat = material_copy_id(default_id_material_unlit);
  vid1 = tex_create(tex_type_image_nomips,tex_format_rgba32);
  tex_set_address(vid1, tex_address_clamp);
  

  //setup hud
  hud_mesh = mesh_gen_plane({1,1},{0,0,1},{0,1,0});
  hud_mat = material_copy_id(default_id_material_unlit);
  hud_tex = tex_create(tex_type_image_nomips,tex_format_rgba32);
  tex_set_address(hud_tex, tex_address_clamp);


  //hide hands, we are not using them right now.
  for (size_t h = 0; h < handed_max; h++) {
      input_hand_visible((handed_)h,false);
  }
  
  //setup textures
  material_set_texture(plane_mat,"diffuse",vid0);
  material_set_texture(plane1_mat,"diffuse",vid1);
  material_set_texture(hud_mat,"diffuse",hud_tex);
  material_set_transparency(hud_mat,transparency_blend);

  //Start your streams first, or the app will block waiting for them to open.
  //You can use vaapih264dec and vaapipostprocess to convert to RGBA in the GPU on AMD cards. Sterokit only supports RGBA32 textures
  std::cout << "Starting video 1" << std::endl;
  bool cap0_open = cap0.open("udpsrc port=5600 caps=application/x-rtp, media=video,clock-rate=90000, encoding-name=H264 ! queue ! rtph264depay ! avdec_h264 ! videoconvert ! video/x-raw,format=RGBA ! appsink max-buffers=2 drop=true sync=false",cv::CAP_GSTREAMER);
  
  if(!cap0.isOpened()){
      std::cout << "Error opening video 1" << std::endl;
      return -1;
  }

  std::cout << "Starting video 2" << std::endl;

  bool cap1_open = cap1.open("udpsrc port=5601 caps=application/x-rtp, media=video,clock-rate=90000, encoding-name=H264 ! queue ! rtph264depay ! avdec_h264 ! videoconvert ! video/x-raw,format=RGBA ! appsink max-buffers=2 drop=true sync=false",cv::CAP_GSTREAMER);
  if(!cap1.isOpened()){
      std::cout << "Error opening video 2" << std::endl;
      return -1;
  }
  
  std::cout << "Starting frame threads" << std::endl;

  std::thread t0(&frame_grabber0);
  std::thread t1(&frame_grabber1);
  std::thread t2(&draw_hud);
  t0.detach();
  t1.detach();
  t2.detach();


  std::cout << "Starting main loop" << std::endl;
  sk_run([]() {
        cv::Mat& frame0 = buffer0[cur_buffer0];
        cv::Mat& frame1 = buffer1[cur_buffer1];
        cv::Mat& hud_fr = hud_buf[cur_hud_buf];
        if(!frame0.empty()){
          tex_set_colors(vid0,frame0.cols,frame0.rows,(void*)frame0.datastart);
        }
  
        if(!frame1.empty()){
          tex_set_colors(vid1,frame1.cols,frame1.rows,(void*)frame1.datastart);
        }
        if(!hud_fr.empty()){
          tex_set_colors(hud_tex,hud_fr.cols,hud_fr.rows,(void*)hud_fr.datastart);
        }

        ui_handle_begin("Plane", plane_pose, mesh_get_bounds(plane_mesh), false);
        render_add_mesh(plane_mesh, plane_mat, matrix_identity);
        ui_handle_end();
        
        ui_handle_begin("Plane1", plane1_pose, mesh_get_bounds(plane1_mesh), false);
        render_add_mesh(plane1_mesh, plane1_mat, matrix_identity);
        ui_handle_end();


        //setup hud
        hud_pose = *input_head();
        hud_pose.position.z -= 0.2f;
        ui_handle_begin("HUD", hud_pose, mesh_get_bounds(hud_mesh), false);
        render_add_mesh(hud_mesh, hud_mat, matrix_trs(hud_pose.position,{0,0,0,1},vec3_one));
        ui_handle_end();
         
        if(cnt % 100 == 0){
            //std::cout << hud_pose.orientation.x << " " << hud_pose.orientation.y << " " << hud_pose.orientation.z << " " << hud_pose.orientation.w << std::endl << std::endl;
            //std::cout << hud_pose.position.x << " " << hud_pose.position.y << " " << hud_pose.position.z << std::endl << std::endl;
            std::cout << vh_att.roll_deg << " " << vh_att.pitch_deg << std::endl;
        }
        
        cnt++;

	});
  t0.join();
  t1.join();
  t2.join();
  cap0.release();
  cap1.release();
  //cap2.release();
  return 0;
}

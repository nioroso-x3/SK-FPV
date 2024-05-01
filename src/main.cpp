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


#define HUD_COLOR cv::Scalar(0,255,0,160)
#define HUD_COLOR_CLR cv::Scalar(0,0,0,0)

//drawing functions to be moved to another file

//draws line, returns origin and end point of the line drawn as a 4 value scalar x1,y1,x2,y2
cv::Scalar 
draw_line(cv::Mat&   mat, //matrix 
          cv::Point  ori,    //origin, either left top or center of the line
          float      length,  //length
          float      angle,  //angle in rads
          int        thc,    // thickness
          cv::Scalar color, // color
          bool       center_ori){ //true for center or false for origin at left top
  if(center_ori){
    cv::Point right;
    cv::Point left;
    right.x = int(ori.x + length/2 * cos(angle) );
    right.y = int(ori.y + length/2 * sin(angle) );
    left.x=(ori.x - (right.x - ori.x));
    left.y=(ori.y - (right.y - ori.y));
    cv::line(mat,left,right,color,thc,cv::LINE_AA);
    return cv::Scalar(left.x,left.y,right.x,right.y);
  }
  else{
    cv::Point right;
    cv::Point left;
    right.x = int(ori.x + length * cos(angle) );
    right.y = int(ori.y + length * sin(angle) );
    cv::line(mat,ori,right,color,thc,cv::LINE_AA);
    return cv::Scalar(ori.x,ori.y,right.x,right.y);
  }
}



//pose_t controls location, X (left/right), Y(up/down), Z(front/back). The second argument is a quartenion, you can search for calculators if you want to change the screen angle and rotation.

//Globals so all threads can access them

//first screen
mesh_t     plane_mesh;
material_t plane_mat;
pose_t     plane_pose = {{0,0.3,-1.5f}, {0,0,0,1}};
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

//Current buffer being used
char cur_buffer0 = 0;
char cur_buffer1 = 0;
char cur_buffer2 = 0;

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



//should make this only one function but whatever
void frame_grabber0()
{
  buffer0[0] = cv::Mat(512,512,CV_8UC4,HUD_COLOR_CLR);
  buffer0[1] = cv::Mat(512,512,CV_8UC4,HUD_COLOR_CLR);

  std::cerr << "Waiting for video 1" << std::endl;

  bool cap0_open = cap0.open("udpsrc port=5600 caps=application/x-rtp, media=video,clock-rate=90000, encoding-name=H264 ! queue ! rtph264depay ! avdec_h264 ! videoconvert ! video/x-raw,format=RGBA ! appsink max-buffers=1 drop=true sync=false",cv::CAP_GSTREAMER);
  
  if(!cap0.isOpened()){
      std::cerr << "Error opening video 1" << std::endl;
  }

  std::cerr << "Starting video 1" << std::endl;
  while(cap0.isOpened()){
    cv::Mat& buf = buffer0[!cur_buffer0];
    cap0.read(buf);
    tex_set_colors(vid0,buf.cols,buf.rows,(void*)buf.datastart);
    cur_buffer0 = !cur_buffer0;
  }
}

void frame_grabber1()
{
  buffer1[0] = cv::Mat(512,512,CV_8UC4,HUD_COLOR_CLR);
  buffer1[1] = cv::Mat(512,512,CV_8UC4,HUD_COLOR_CLR);

  std::cerr << "Waiting for video 2" << std::endl;

  bool cap1_open = cap1.open("udpsrc port=5601 caps=application/x-rtp, media=video,clock-rate=90000, encoding-name=H264 ! queue ! rtph264depay ! avdec_h264 ! videoconvert ! video/x-raw,format=RGBA ! appsink max-buffers=1 drop=true sync=false",cv::CAP_GSTREAMER);
  
  if(!cap1.isOpened()){
      std::cerr << "Error opening video 2" << std::endl;
  }

  std::cerr << "Starting video 2" << std::endl;
  while(cap1.isOpened()){
    cv::Mat& buf = buffer1[!cur_buffer1];
    cap1.read(buf);
    tex_set_colors(vid1,buf.cols,buf.rows,(void*)buf.datastart);
    cur_buffer1 = !cur_buffer1;
  }
}

void mavlink_thread()
{
  std::cerr << "Started mavlink setup thread" << std::endl;
  Mavsdk mavsdk{Mavsdk::Configuration{Mavsdk::ComponentType::GroundStation}};
  ConnectionResult connection_result = mavsdk.add_any_connection("udp://0.0.0.0:14551");

  if (connection_result != ConnectionResult::Success) {
      std::cerr << "Connection failed: " << connection_result << '\n';
  }

  auto system = mavsdk.first_autopilot(3.0);
  while(!system) {
      std::cerr << "Connection timeout, waiting 5 seconds " << std::endl;
      system = mavsdk.first_autopilot(5.0);
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
     if (bat.id == 0){
        vh_bat0 = bat;
     }
     if (bat.id == 1){
        vh_bat1 = bat;
     }
  });
  telemetry.subscribe_fixedwing_metrics([](Telemetry::FixedwingMetrics fwing){
     vh_fwing = fwing;
  });
  telemetry.subscribe_gps_info([](Telemetry::GpsInfo gpsi){
     vh_gpsi = gpsi;
  });
   telemetry.subscribe_raw_gps([](Telemetry::RawGps gpsr){
     vh_gpsr = gpsr;
  });
 

  telemetry.subscribe_status_text([](Telemetry::StatusText st_text){
     if (st_text.text != vh_st_text.text){
       vh_st_text = st_text;
       status_counter = 20*15;
     }
  });


  std::cerr << "Mavlink thread going to sleep" << std::endl;
  while(true){
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }


}

void draw_hud()
{


  cv::Mat buf = cv::Mat(1024,1024,CV_8UC4,HUD_COLOR_CLR);

  std::cerr << "Starting HUD thread" << std::endl;
  float r,p = 0;
  while(true){
      //clear buffer
      buf.setTo(HUD_COLOR_CLR);
      //copy current values
      Telemetry::Position c_pos = vh_pos;
      Telemetry::EulerAngle c_att = vh_att;
      Telemetry::Battery c_bat0 = vh_bat0;
      Telemetry::Battery c_bat1 = vh_bat1;
      Telemetry::FixedwingMetrics c_fw = vh_fwing;
      Telemetry::RawGps c_gpsr = vh_gpsr;
      Telemetry::StatusText st_text = vh_st_text;

      
      //draw roll
      r = -c_att.roll_deg * M_PI /180.0;
      draw_line(buf,cv::Point(512,512),512,r,2,HUD_COLOR,true);
      draw_line(buf,cv::Point(512,512),96,r,4,HUD_COLOR_CLR,true);
      //draw pitch
      p = c_att.pitch_deg * M_PI / 180.0;



      //using text as placeholders
      cv::putText(buf,"PITCH:",cv::Point(100,512),cv::FONT_HERSHEY_DUPLEX,1.0,HUD_COLOR,2);
      cv::putText(buf,std::to_string(int(c_att.pitch_deg)),cv::Point(210,512),cv::FONT_HERSHEY_DUPLEX,1.0,HUD_COLOR,2);

      cv::putText(buf,"ALT:",cv::Point(770,512),cv::FONT_HERSHEY_DUPLEX,1.0,HUD_COLOR,2);
      cv::putText(buf,std::to_string(c_pos.relative_altitude_m),cv::Point(880,512),cv::FONT_HERSHEY_DUPLEX,1.0,HUD_COLOR,2);
 
      cv::putText(buf,"AS:",cv::Point(770,542),cv::FONT_HERSHEY_DUPLEX,1.0,HUD_COLOR,2);
      cv::putText(buf,std::to_string(c_fw.airspeed_m_s),cv::Point(880,542),cv::FONT_HERSHEY_DUPLEX,1.0,HUD_COLOR,2);
 
      cv::putText(buf,"GS:",cv::Point(770,572),cv::FONT_HERSHEY_DUPLEX,1.0,HUD_COLOR,2);
      cv::putText(buf,std::to_string(c_gpsr.velocity_m_s),cv::Point(880,572),cv::FONT_HERSHEY_DUPLEX,1.0,HUD_COLOR,2);
    
      
      cv::putText(buf,"VOLT0:",cv::Point(10,800),cv::FONT_HERSHEY_DUPLEX,1.0,HUD_COLOR,2);
      cv::putText(buf,std::to_string(c_bat0.voltage_v),cv::Point(120,800),cv::FONT_HERSHEY_DUPLEX,1.0,HUD_COLOR,2);

      cv::putText(buf," AMP0:",cv::Point(10,830),cv::FONT_HERSHEY_DUPLEX,1.0,HUD_COLOR,2);
      cv::putText(buf,std::to_string(c_bat0.current_battery_a),cv::Point(120,830),cv::FONT_HERSHEY_DUPLEX,1.0,HUD_COLOR,2);

      cv::putText(buf,"VOLT1:",cv::Point(10,860),cv::FONT_HERSHEY_DUPLEX,1.0,HUD_COLOR,2);
      cv::putText(buf,std::to_string(c_bat1.voltage_v),cv::Point(120,860),cv::FONT_HERSHEY_DUPLEX,1.0,HUD_COLOR,2);

      cv::putText(buf," AMP1:",cv::Point(10,890),cv::FONT_HERSHEY_DUPLEX,1.0,HUD_COLOR,2);
      cv::putText(buf,std::to_string(c_bat1.current_battery_a),cv::Point(120,890),cv::FONT_HERSHEY_DUPLEX,1.0,HUD_COLOR,2);

      //draw message if needed      
      if (status_counter){
        status_counter -= 1;

      }

      tex_set_colors(hud_tex,buf.cols,buf.rows,(void*)buf.datastart);

      std::this_thread::sleep_for(std::chrono::milliseconds(50));

   }


}



model_t main_screen;







int main(int argc, char* argv[]) {

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


  std::cout << "Starting threads" << std::endl;
  std::thread t0(&frame_grabber0);
  std::thread t1(&frame_grabber1);
  std::thread t2(&draw_hud);
  std::thread t3(&mavlink_thread);
  t0.detach();
  t1.detach();
  t2.detach();
  t3.detach();


  std::cout << "Starting main loop" << std::endl;
  sk_run([]() {
        //video plane 1
        ui_handle_begin("Plane", plane_pose, mesh_get_bounds(plane_mesh), false);
        render_add_mesh(plane_mesh, plane_mat, matrix_identity);
        ui_handle_end();
        
        //video plane 2
        ui_handle_begin("Plane1", plane1_pose, mesh_get_bounds(plane1_mesh), false);
        render_add_mesh(plane1_mesh, plane1_mat, matrix_identity);
        ui_handle_end();

        //setup hud position and orientation. HUD is always 1 unit in front of camera, facing it.
        vec3 at = input_head()->position + input_head()->orientation * vec3_forward * 1.0f;
        quat ori = quat_lookat(input_head()->position,at);
        //HUD plane
        ui_handle_begin("HUD", hud_pose, mesh_get_bounds(hud_mesh), false);
        render_add_mesh(hud_mesh, hud_mat, matrix_trs(at,ori,vec3_one));
        ui_handle_end();
         
        //print some debug stuff if needed
        if(cnt % 100 == 0){
            //std::cout << hud_pose.orientation.x << " " << hud_pose.orientation.y << " " << hud_pose.orientation.z << " " << hud_pose.orientation.w << std::endl << std::endl;
            //std::cout << hud_pose.position.x << " " << hud_pose.position.y << " " << hud_pose.position.z << std::endl << std::endl;
            //std::cout << vh_att.roll_deg << " " << vh_att.pitch_deg << std::endl;
            //std::cout << ori.x << " " << ori.y << " " << ori.z << " "<< ori.w << std::endl;
        }
        
        cnt++;

	});
  t0.join();
  t1.join();
  t2.join();
  t3.join();
  cap0.release();
  cap1.release();
  //cap2.release();
  return 0;
}

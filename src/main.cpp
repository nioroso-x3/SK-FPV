#include "common.h"
#include "hud.h"
using namespace sk;
using namespace mavsdk;


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
    right.x = int(ori.x + length * cos(angle) );
    right.y = int(ori.y + length * sin(angle) );
    cv::line(mat,ori,right,color,thc,cv::LINE_AA);
    return cv::Scalar(ori.x,ori.y,right.x,right.y);
  }
}


//should make this only one function but whatever
void frame_grabber0()
{
  buffer0[0] = cv::Mat(512,512,CV_8UC4,HUD_COLOR_CLR);
  buffer0[1] = cv::Mat(512,512,CV_8UC4,HUD_COLOR_CLR);

  std::cerr << "Waiting for video 1" << std::endl;
  cv::VideoCapture cap;
  bool cap_open = cap.open("udpsrc port=5600 caps=application/x-rtp, media=video,clock-rate=90000, encoding-name=H264 ! queue ! rtph264depay ! avdec_h264 ! videoconvert ! video/x-raw,format=RGBA ! appsink max-buffers=1 drop=true sync=false",cv::CAP_GSTREAMER);
  
  if(!cap.isOpened()){
      std::cerr << "Error opening video 1" << std::endl;
  }

  std::cerr << "Starting video 1" << std::endl;
  while(cap.isOpened()){
    cv::Mat& buf = buffer0[!cur_buffer0];
    cap.read(buf);
    tex_set_colors(vid0,buf.cols,buf.rows,(void*)buf.datastart);
    cur_buffer0 = !cur_buffer0;
  }
  cap.release();
  //something crashed the decoder
}

void frame_grabber1()
{
  buffer1[0] = cv::Mat(512,512,CV_8UC4,HUD_COLOR_CLR);
  buffer1[1] = cv::Mat(512,512,CV_8UC4,HUD_COLOR_CLR);

  std::cerr << "Waiting for video 2" << std::endl;
  cv::VideoCapture cap;
  bool cap_open = cap.open("udpsrc port=5601 caps=application/x-rtp, media=video,clock-rate=90000, encoding-name=H264 ! queue ! rtph264depay ! avdec_h264 ! videoconvert ! video/x-raw,format=RGBA ! appsink max-buffers=1 drop=true sync=false",cv::CAP_GSTREAMER);
  
  if(!cap.isOpened()){
      std::cerr << "Error opening video 2" << std::endl;
  }

  std::cerr << "Starting video 2" << std::endl;
  while(cap.isOpened()){
    cv::Mat& buf = buffer1[!cur_buffer1];
    cap.read(buf);
    tex_set_colors(vid1,buf.cols,buf.rows,(void*)buf.datastart);
    cur_buffer1 = !cur_buffer1;
  }
  cap.release();
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
      std::cerr << "Connection timeout, waiting 30 seconds " << std::endl;
      system = mavsdk.first_autopilot(30.0);
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
    //maybe do some health checks in this loop
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }


}

void draw_hud()
{
  cv::Mat buf = cv::Mat(1024,1024,CV_8UC4,HUD_COLOR_CLR);

  std::cerr << "Starting HUD thread" << std::endl;
  while(true){
      draw_cairo_hud();
      std::this_thread::sleep_for(std::chrono::milliseconds(50));

   }
}


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
  hud_mesh = mesh_gen_plane({1.5,1},{0,0,1},{0,1,0});
  hud_mat = material_copy_id(default_id_material_unlit);
  hud_tex = tex_create(tex_type_image_nomips,tex_format_rgba32_linear);
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
  //cap2.release();
  return 0;
}

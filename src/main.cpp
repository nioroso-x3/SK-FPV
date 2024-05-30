#include "common.h"
#include "hud.h"
#include "mavlink_setup.h"
#include "stabilization.h"

using namespace sk;
using namespace mavsdk;

//should make this only one function but whatever
void frame_grabber0()
{
  std::cerr << "Waiting for video 1" << std::endl;
  cv::VideoCapture cap;
  bool cap_open = cap.open("udpsrc port=5600 caps=application/x-rtp, media=video,clock-rate=90000, encoding-name=H264 ! queue ! rtph264depay ! avdec_h264 ! videoconvert ! video/x-raw,format=RGBA ! appsink max-buffers=50 drop=false sync=false",cv::CAP_GSTREAMER);
  
  if(!cap.isOpened()){
      std::cerr << "Error opening video 1" << std::endl;
  }

  std::cerr << "Starting video 1" << std::endl;
  cv::Mat buf;
  cv::Mat st_buf;
  bool run_stab = true;
  stabilizer stab;
  while(cap.isOpened()){
    cap.read(buf);
    if (run_stab){
      stab.stabilize(buf,st_buf);
      tex_set_colors(vid0,st_buf.cols,st_buf.rows,(void*)st_buf.datastart);
    }
    else{
      tex_set_colors(vid0,buf.cols,buf.rows,(void*)buf.datastart);
    }
  }
  cap.release();
  //something crashed the decoder
}

void frame_grabber1()
{
  std::cerr << "Waiting for video 2" << std::endl;
  cv::VideoCapture cap;
  bool cap_open = cap.open("udpsrc port=5601 caps=application/x-rtp, media=video,clock-rate=90000, encoding-name=H264 ! queue ! rtph264depay ! avdec_h264 ! videoconvert ! video/x-raw,format=RGBA ! appsink max-buffers=1 drop=true sync=false",cv::CAP_GSTREAMER);
  
  if(!cap.isOpened()){
      std::cerr << "Error opening video 2" << std::endl;
  }

  std::cerr << "Starting video 2" << std::endl;
  cv::Mat buf;
  while(cap.isOpened()){
    cap.read(buf);
    tex_set_colors(vid1,buf.cols,buf.rows,(void*)buf.datastart);
   
  }
  cap.release();
}

void frame_grabber2()
{
  std::cerr << "Waiting for video 3" << std::endl;
  cv::VideoCapture cap;
  bool cap_open = cap.open("udpsrc port=5602 caps=application/x-rtp, media=video,clock-rate=90000, encoding-name=H264 ! queue ! rtph264depay ! avdec_h264 ! videoconvert ! video/x-raw,format=RGBA ! appsink max-buffers=1 drop=true sync=false",cv::CAP_GSTREAMER);
  
  if(!cap.isOpened()){
      std::cerr << "Error opening video 3" << std::endl;
  }

  std::cerr << "Starting video 3" << std::endl;



  cv::Mat buf;
  cv::Mat crop;
  int w;
  int h;
  int w2;
  cv::Rect grayIR;
  cv::Rect colorRGB;
  while(cap.isOpened()){
    cap.read(buf);
    w = buf.cols;
    w2 = int(w/2);
    h = buf.rows;
    grayIR = cv::Rect(0,0,w2,h);
    colorRGB = cv::Rect(w2,0,w2,h);
    if (gnd_cam_color) crop = buf(colorRGB).clone();
    else crop = buf(grayIR).clone();
    tex_set_colors(vid2,crop.cols,crop.rows,(void*)crop.datastart);
   
  }
  cap.release();
}


void skybox_thread(){
  //this will grab video from the 360 cameras and convert it to a equirect cubemap
  

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


int main(int argc, char* argv[]) {

  //initialize stereokit
  sk_settings_t settings = {};
	
  settings.app_name           = "SK_FPV";
	settings.assets_folder      = "Assets";
	settings.display_preference = display_mode_mixedreality;
	
  if (!sk_init(settings))
		return 1;
  
  //setup first screen, we generate a plane with 16:9 ratio
  plane_mesh = mesh_gen_plane({1.777f*p1s,p1s},{0,0,1},{0,1,0});
  plane_mat = material_copy_id(default_id_material_unlit);
  vid0 = tex_create(tex_type_image_nomips,tex_format_rgba32);
  tex_set_address(vid0, tex_address_clamp);

  
  //setup second screen, we generate a plane with a 4:3 ratio
  plane1_mesh = mesh_gen_plane({1.25f*p2s,p2s},{0,0,1},{0,1,0});
  plane1_mat = material_copy_id(default_id_material_unlit);
  vid1 = tex_create(tex_type_image_nomips,tex_format_rgba32);
  tex_set_address(vid1, tex_address_clamp);
  
  //setup third screen, we generate a plane with a 4:3 ratio
  plane2_mesh = mesh_gen_plane({1.25f*p3s,p3s},{0,0,1},{0,1,0});
  plane2_mat = material_copy_id(default_id_material_unlit);
  vid2 = tex_create(tex_type_image_nomips,tex_format_rgba32);
  tex_set_address(vid2, tex_address_clamp);

  //setup hud
  hud_mesh = mesh_gen_plane({1.5f*hud_s,hud_s},{0,0,1},{0,1,0});
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
  material_set_texture(plane2_mat,"diffuse",vid2);
  material_set_texture(hud_mat,"diffuse",hud_tex);
  material_set_transparency(hud_mat,transparency_blend);


  std::cout << "Starting threads" << std::endl;
  std::thread t0(&frame_grabber0);
  std::thread t1(&frame_grabber1);
  std::thread t2(&frame_grabber2);
  std::thread t3(&draw_hud);
  std::thread t4(&mavlink_thread);
  t0.detach();
  t1.detach();
  t2.detach();
  t3.detach();
  t4.detach();


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
        
        //video plane 3
        ui_handle_begin("Plane2", plane2_pose, mesh_get_bounds(plane2_mesh), false);
        render_add_mesh(plane2_mesh, plane2_mat, matrix_identity);
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
  t4.join();
  return 0;
}

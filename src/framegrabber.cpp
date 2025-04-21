#include "framegrabber.h"


FrameWriter::FrameWriter(){}
FrameWriter::FrameWriter(std::string name, 
                         std::string gst_pipeline,
                         std::map<std::string,cv::Mat> &overlays,
                         std::map<std::string,cv::Size> &vsizes,
                         tex_t       &vid0, 
                         tex_t       &vid1, 
                         bool        stab, 
                         cv::Mat     K, 
                         cv::Mat     D, 
                         float       balance, 
                         cv::Size    corr_ori, 
                         int         type){
    start(name, gst_pipeline, overlays, vsizes, vid0, vid1, stab, K, D, balance, corr_ori, type);
}
void FrameWriter::play(){
    playing = true;
}
void FrameWriter::start(std::string  name, 
                        std::string  gst_pipeline, 
                        std::map<std::string,cv::Mat> &overlays,
                        std::map<std::string,cv::Size> &vsizes,
                        tex_t        &vid0, 
                        tex_t        &vid1, 
                        bool         stab, 
                        cv::Mat      K, 
                        cv::Mat      D, 
                        float        balance, 
                        cv::Size     corr_ori, 
                        int          type){
    this->name = name;
    this->gst_pipeline = gst_pipeline;
    this->stab = stab;
    this->corr_ori = corr_ori;
    this->K = K;
    this->D = D;
    this->balance = balance;
    this->type = type;
    playing = true;
    run = true;
    t = std::make_shared<std::thread>(&FrameWriter::stream, this, &vid0, &vid1, &overlays, &vsizes);
    t->detach();
}


void FrameWriter::stop(){
   playing = false;
}
void FrameWriter::terminate(){
   stop();
   run = false;
   std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

void FrameWriter::toggleStab(){
   stab = !stab;
}

void FrameWriter::stream(tex_t *vid0, 
                         tex_t *vid1, 
                         std::map<std::string,cv::Mat> *overlays, 
                         std::map<std::string,cv::Size> *vsizes){
   cv::VideoCapture cap(gst_pipeline, cv::CAP_GSTREAMER);
   cv::Mat img;
   cv::Mat i1;
   cv::Mat i2;
   stabilizer stab0;
   stabilizer stab1;
   std::cout << "Starting pipeline:" << std::endl;
   std::cout << "  " << gst_pipeline << std::endl;
   std:: cout << "  Type: " << type << std::endl;
   while(cap.isOpened() && run){
      if(!playing){
         std::this_thread::sleep_for(std::chrono::milliseconds(100));
         continue;
      }
      cap.read(img);
      (*vsizes)[name] = cv::Size(img.cols,img.rows);
      if (overlays->find(name) != overlays->end()){
        img = img + overlays->at(name);
      }
     
      //dual image
      if (type == 1){
          int w = img.cols;
          int w2 = int(w/2);
          int h = img.rows;
          cv::Rect r1 = cv::Rect(0,0,w2,h);
          cv::Rect r2 = cv::Rect(w2,0,w2,h);
          i1 = img(r1).clone();
          i2 = img(r2).clone();
          if (stab){
              std::thread t0(&stabilizer::stabilize_i, &stab0, std::ref(i1));
              stab1.stabilize_i(i2);
              t0.join();
              std::future<cv::Mat> f1 = std::async(std::launch::async, &stabilizer::getStabFrame, &stab0);
              i2 = stab1.getStabFrame();
              i1 = f1.get();
         }
          
          tex_set_colors(*vid0,i1.cols,i1.rows,(void*)i1.datastart);
          tex_set_colors(*vid1,i2.cols,i2.rows,(void*)i2.datastart);
      }
      //single video
      else{
          if (stab){
              stab0.stabilize_i(img);
              img = stab0.getStabFrame();                  
          }
          tex_set_colors(*vid0,img.cols,img.rows,(void*)img.datastart);

      }
   }
   cap.release();
}

FrameWriter::~FrameWriter(){
    terminate();
}

VideoContainer::VideoContainer(){}
VideoContainer::VideoContainer(std::string jsonpath){load_file(jsonpath);}
VideoContainer::VideoContainer(nlohmann::json json){load_json(json);}

void VideoContainer::add_surface(std::string name, float ratio, float scale, pose_t pose, bool transparent) {
    //setup first screen, we generate a plane with 16:9 ratio
    mesh_ts[name] = mesh_gen_plane({ratio*scale,scale},{0,0,1},{0,1,0});
    material_ts[name] = material_copy_id(default_id_material_unlit);
    pose_ts[name] = pose;
    tex_ts[name] = tex_create(tex_type_image_nomips | tex_type_dynamic, tex_format_rgba32);

    tex_set_address(tex_ts[name], tex_address_clamp);
    material_set_texture(material_ts[name],"diffuse", tex_ts[name]);
    if(transparent) material_set_transparency(material_ts[name],transparency_blend);
}

void VideoContainer::add_undistortion_shader(std::string name, const cv::Mat K, const cv::Mat D, const cv::Size imageSize, double balance){
    //load the shader
    shader_t undistort = shader_create_file("undistort.hlsl");
    material_set_shader(material_ts[name], undistort);
    shader_release(undistort);
                                  
    //calculate the transform matrix
    cv::Mat map1, map2;
    precomputeUndistortMaps(K, D, imageSize, balance, map1, map2);
    
    //normalize
    map1 = map1 / (map1.cols-1); 
    map2 = map2 / (map2.rows-1);
    
    //Create the lookup textures
    std::string fx = name + std::string("_distx");
    std::string fy = name + std::string("_disty");
  
    tex_ts[fx] = tex_create(tex_type_image_nomips, tex_format_r32);
    tex_ts[fy] = tex_create(tex_type_image_nomips, tex_format_r32);
    
    //Configure the textures
    tex_set_address(tex_ts[fx], tex_address_clamp);
    tex_set_address(tex_ts[fy], tex_address_clamp);
        
    //Bind the textures to the shader
    material_set_texture(material_ts[name],"mapX",tex_ts[fx]);
    material_set_texture(material_ts[name],"mapY",tex_ts[fy]);
        
    //Write the transformation matrixes to the respective lookup textures.
    tex_set_colors(tex_ts[fx],map1.cols,map1.rows,(void*)map1.datastart);
    tex_set_colors(tex_ts[fy],map2.cols,map2.rows,(void*)map2.datastart);
      
}     

void VideoContainer::del_surface(std::string name) {
    if (frame_caps.find(name) != frame_caps.end()){
      frame_caps[name].terminate();
      frame_caps.erase(name);
    }
    std::string oname = name + std::string("_1");
    mesh_ts.erase(name);
    material_ts.erase(name);
    pose_ts.erase(name);
    tex_ts.erase(name);
    mesh_ts.erase(oname);
    material_ts.erase(oname);
    pose_ts.erase(oname);
    tex_ts.erase(oname);

}
    
std::vector<std::string> VideoContainer::list_names() {
    std::vector<std::string> keys;
    for(auto iter = mesh_ts.begin(); iter != mesh_ts.end(); ++iter)
    {
        std::string k =  iter->first;
        keys.push_back(k);
    }
    return keys;
}
    
void VideoContainer::load_file(std::string jsonpath) {
    std::ifstream file(jsonpath.c_str(),std::ifstream::in);
    if (file.is_open()) {
        nlohmann::json j;
        file >> j;
        load_json(j);
    }
}
    
void VideoContainer::load_json(nlohmann::json j) {
    try {
        for (auto& [name, obj_json] : j.items()) {
            std::string oname = name + std::string("_1");
            std::string pipeline = obj_json.at("pipeline").get<std::string>();
            float aspect_ratio = obj_json.at("aspect_ratio").get<float>();
            float scale = obj_json.at("scale").get<float>();
            int stab = obj_json.at("stabilize").get<int>();
            int type = obj_json.at("type").get<int>();
            float balance = 0.0f;
            cv::Size corr_ori;
            if ((0 > type) && (type > 1)) continue; 
            std::array<float, 3> pt0;
            std::array<float, 4> pr0;
            std::array<float, 3> pt1;
            std::array<float, 4> pr1;
            cv::Mat K;
            cv::Mat D;
            bool distorted = false;
            if(obj_json.contains("K") && obj_json.contains("D") && obj_json.contains("balance") && obj_json.contains("corr_ori") ) distorted = true;
            //pose
            if (!obj_json.at("pose0").is_array() || obj_json.at("pose0").size() != 2)
                continue;
            pt0 = obj_json.at("pose0")[0].get<std::array<float, 3>>();
            pr0 = obj_json.at("pose0")[1].get<std::array<float, 4>>();
            if(distorted){
                std::cout << "Distortion correction detected\n";
                std::array<double, 9> Ka = obj_json.at("K").get<std::array<double,9>>();
                std::array<double, 4> Da = obj_json.at("D").get<std::array<double,4>>();
                K = cv::Mat(3,3,CV_64F,Ka.data()).clone();
                D = cv::Mat(4,1,CV_64F,Da.data()).clone();
                balance = obj_json.at("balance").get<float>();
                std::array<int, 2> corr_ori_a = obj_json.at("corr_ori").get<std::array<int,2>>();
                corr_ori = cv::Size(corr_ori_a[0],corr_ori_a[1]);
            }
            if (type == 1){
                //pose1
                if (!obj_json.at("pose1").is_array() || obj_json.at("pose1").size() != 2)
                    continue;
                pt1 = obj_json.at("pose1")[0].get<std::array<float, 3>>();
                pr1 = obj_json.at("pose1")[1].get<std::array<float, 4>>();
                pose_t pose = {{pt1[0],pt1[1],pt1[2]}, {pr1[0],pr1[1],pr1[2],pr1[3]}};
                add_surface(oname, aspect_ratio, scale, pose, false);

            }
            pose_t pose = {{pt0[0],pt0[1],pt0[2]}, {pr0[0],pr0[1],pr0[2],pr0[3]}};
            add_surface(name, aspect_ratio, scale, pose, false);
            tex_t &t0 = tex_ts[name];
            tex_t &t1 = type == 0 ? tex_ts[name] : tex_ts[oname];
            //disable stabilization, we have to build a shader that does all transformations first...
            if(distorted){
                stab = false;
                add_undistortion_shader(name, K, D, corr_ori, balance);
                if(type==1) add_undistortion_shader(oname, K, D, corr_ori, balance);
            }
            frame_caps[name].start(name, pipeline, overlays, vsizes, t0, t1, (bool)stab, K, D, balance, corr_ori, type);
        }
    }
    catch (const nlohmann::json::exception& e) {
        std::cerr << "JSON parsing error: " << e.what() << std::endl;
    }
}

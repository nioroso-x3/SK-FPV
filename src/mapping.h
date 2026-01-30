#ifndef MAPPING_H
#define MAPPING_H

#include <map>
#include <opencv2/opencv.hpp>
#include <mbgl/map/map.hpp>
#include <mbgl/map/map_options.hpp>
#include <mbgl/util/image.hpp>
#include <mbgl/util/run_loop.hpp>
#include <mbgl/gfx/backend.hpp>
#include <mbgl/gfx/headless_frontend.hpp>
#include <mbgl/style/style.hpp>
#include <mbgl/style/layers/symbol_layer.hpp>
#include <mbgl/style/sources/geojson_source.hpp>
#include <mbgl/util/geojson.hpp>
#include <mbgl/util/premultiply.hpp>
#include <nlohmann/json.hpp>

#define MAP_W 512
#define MAP_H 512

class maplibre{
  public:
    maplibre(std::string apikey,std::string styleurl);
    ~maplibre();
    int set_point(float lat, float lon, int id, std::string iconName);
    int delete_point(int id);
    int set_home(float lat, float lon);
    int get_map(cv::Mat* output, double lat, double lon, double bearing, double zoom);
    void add_icon(std::string name, std::string pngpath);
  private:
    mbgl::util::RunLoop loop;
    mbgl::HeadlessFrontend frontend;
    mbgl::Map map_obj;
    std::map<int, mbgl::Point<float>> landmarks;
};

#endif

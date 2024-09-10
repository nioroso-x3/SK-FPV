#include <opencv2/opencv.hpp>
#include <mbgl/map/map.hpp>
#include <mbgl/map/map_options.hpp>
#include <mbgl/util/image.hpp>
#include <mbgl/util/run_loop.hpp>

#include <mbgl/gfx/backend.hpp>
#include <mbgl/gfx/headless_frontend.hpp>
#include <mbgl/style/style.hpp>

class maplibre{
  public:
    maplibre(std::string apikey,std::string styleurl);
    ~maplibre();
    int get_map(cv::Mat* output, double lat, double lon, double bearing, double zoom);

  private:
    mbgl::util::RunLoop loop;
    mbgl::HeadlessFrontend frontend;
    mbgl::Map map;
};

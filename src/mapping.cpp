
#include "mapping.h"

using namespace mbgl;

maplibre::maplibre(std::string apikey,std::string styleurl) :
 loop(),
 frontend({512,512},1.0f),
 map(frontend,
     MapObserver::nullObserver(),
     MapOptions()
                .withMapMode(MapMode::Static)
                .withSize(frontend.getSize())
                .withPixelRatio(1.0f),
            ResourceOptions()
                .withCachePath(std::string("cache.sqlite"))
                .withAssetPath(std::string("."))
                .withApiKey(apikey)
                .withTileServerOptions(mbgl::TileServerOptions::MapTilerConfiguration()))
{
    map.getStyle().loadURL(styleurl);
}

maplibre::~maplibre(){
}

int maplibre::get_map(cv::Mat* output, double lat, double lon, double bearing, double zoom) {
    //avoid NaN, set to 0
    if (lat != lat) lat = 0.0;
    if (lon != lon) lon = 0.0;
    if (bearing != bearing) bearing = 0;
    double pitch = 0;
    map.jumpTo(CameraOptions().withCenter(LatLng{lat, lon}).withZoom(zoom).withBearing(bearing).withPitch(pitch));

    try {
        auto image = frontend.render(map).image;
        uint32_t w = image.size.width;
        uint32_t h = image.size.width;
        cv::Mat tile(w,h,CV_8UC4,image.data.get());
        *output = tile.clone();
        
    } catch (std::exception& e) {
        return -1;
    }

    return 0;
}

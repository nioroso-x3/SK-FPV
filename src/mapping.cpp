#include "mapping.h"

using namespace mbgl;

maplibre::maplibre(std::string apikey,std::string styleurl) :
 loop(),
 frontend({MAP_W,MAP_H},1.0f),
 map_obj(frontend,
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
    //set style from JSON file
    map_obj.getStyle().loadURL(styleurl);
    //render an image to finish starting up the map object
    auto image = frontend.render(map_obj).image;
    //add support for dynamic landmarks from MAVLINK stream, like the waypoints
    auto landmarkSource = std::make_unique<style::GeoJSONSource>("landmarks");
    map_obj.getStyle().addSource(std::unique_ptr<style::Source>(std::move(landmarkSource)));
     
    // Create a symbol layer for rendering the landmarks
    auto landmarkLayer = std::make_unique<style::SymbolLayer>("landmark-layer", "landmarks");
    landmarkLayer->setIconImage(style::PropertyValue<style::expression::Image>("circle"));
    landmarkLayer->setIconSize(1.0f);
    landmarkLayer->setVisibility(style::VisibilityType::Visible);
    landmarkLayer->setMinZoom(0);
    landmarkLayer->setMaxZoom(22);
    map_obj.getStyle().addLayer(std::unique_ptr<style::Layer,std::default_delete<style::Layer>>(std::move(landmarkLayer)));

    //keep the home position separate
    auto homeSource = std::make_unique<style::GeoJSONSource>("home");
    map_obj.getStyle().addSource(std::unique_ptr<style::Source>(std::move(homeSource)));
     
    // Create a symbol layer for rendering the home position
    auto homeLayer = std::make_unique<style::SymbolLayer>("home-layer", "home");
    homeLayer->setIconImage(style::PropertyValue<style::expression::Image>("home"));
    homeLayer->setIconSize(1.0f);
    homeLayer->setVisibility(style::VisibilityType::Visible);
    homeLayer->setMinZoom(0);
    homeLayer->setMaxZoom(22);
    map_obj.getStyle().addLayer(std::unique_ptr<style::Layer,std::default_delete<style::Layer>>(std::move(homeLayer)));


}

maplibre::~maplibre(){
}


int maplibre::set_point(float lat, float lon, int id, std::string iconName) {
    style::GeoJSONSource *source = (style::GeoJSONSource*)map_obj.getStyle().getSource("landmarks");

    if (lon != lon || lat != lat) return -1;

    // Create or update the landmark with the icon property

    landmarks[id] = Point<float>(lon, lat);

    // Update the GeoJSON source with the new set of landmarks
    nlohmann::json features = nlohmann::json::array();
    for (const auto& [id, point] : landmarks) {
        nlohmann::json feature = {
            {"type", "Feature"},
            {"properties", {
                {"id", id},
                {"icon", "circle"}  // Set icon name for this landmark
            }},
            {"geometry", {
                {"type", "Point"},
                {"coordinates", {point.x, point.y}}
            }}
        };
        features.push_back(feature);
    }
    nlohmann::json geojson = {
        {"type", "FeatureCollection"},
        {"features", features}
    };
    std::string strjson = geojson.dump();
    auto parsed = mapbox::geojson::parse(strjson);
    //std::cout << "Parsed\n";
    source->setGeoJSON(parsed);
    return 0;
}

int maplibre::set_home(float lat, float lon) {
    style::GeoJSONSource *source = (style::GeoJSONSource*)map_obj.getStyle().getSource("home");

    if (lon != lon || lat != lat) return -1;

    // Create or update the landmark with the icon property

    // Update the GeoJSON source with the new set of landmarks
    nlohmann::json features = nlohmann::json::array();
    nlohmann::json feature = {
            {"type", "Feature"},
            {"properties", {
                {"id", 0},
                {"icon", "home"}  // Set icon name for this landmark
            }},
            {"geometry", {
                {"type", "Point"},
                {"coordinates", {lon, lat}}
            }}
        };
    features.push_back(feature);
    nlohmann::json geojson = {
        {"type", "FeatureCollection"},
        {"features", features}
    };
    std::string strjson = geojson.dump();
    auto parsed = mapbox::geojson::parse(strjson);
    //std::cout << "Parsed\n";
    source->setGeoJSON(parsed);
    return 0;
}


void maplibre::add_icon(std::string name, std::string pngpath){
    cv::Mat image = cv::imread(pngpath, cv::IMREAD_UNCHANGED);
    // Check if the image has an alpha channel; if not, add one
    if (image.channels() == 3) {
        // Convert BGR to BGRA by adding an alpha channel of full opacity (255)
        cv::Mat bgra;
        cv::cvtColor(image, bgra, cv::COLOR_BGR2BGRA);
        image = bgra;
    }
    uint32_t width = image.cols;
    uint32_t height = image.rows;
    PremultipliedImage premultipliedImage({width, height});

    // Copy the image data to premultipliedImage's buffer
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            cv::Vec4b& pixel = image.at<cv::Vec4b>(y, x);  // BGRA format in OpenCV
            std::size_t index = (y * width + x) * 4;
            premultipliedImage.data[index + 0] = pixel[0];  // Blue
            premultipliedImage.data[index + 1] = pixel[1];  // Green
            premultipliedImage.data[index + 2] = pixel[2];  // Red
            premultipliedImage.data[index + 3] = pixel[3];  // Alpha
        }
    }
 // Create a MapLibre Image and add it to the map's style
    auto mapImage = std::make_unique<mbgl::style::Image>(
        name,   // The name of the icon
        std::move(premultipliedImage),
        1.0f  // Pixel ratio (adjust if you have a high DPI image)
    );

    // Add the image to the map's sprite atlas
    map_obj.getStyle().addImage(std::move(mapImage));
}


int maplibre::get_map(cv::Mat* output, double lat, double lon, double bearing, double zoom) {
    //avoid NaN, set to 0
    if (lat != lat) lat = 0.0;
    if (lon != lon) lon = 0.0;
    if (bearing != bearing) bearing = 0;
    map_obj.jumpTo(CameraOptions().withCenter(LatLng{lat, lon}).withZoom(zoom).withBearing(bearing).withPitch(0.0));

    try {
        auto image = util::unpremultiply(frontend.render(map_obj).image);
        cv::Mat tile(image.size.width,image.size.height,CV_8UC4,image.data.get());
        *output = tile.clone();
        
    } catch (std::exception& e) {
        return -1;
    }

    return 0;
}


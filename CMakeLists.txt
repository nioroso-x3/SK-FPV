# To use this template, 
cmake_minimum_required(VERSION 3.11)
project(SK_FPV VERSION "0.0.1" LANGUAGES CXX C)

# Grab and build StereoKit from the GitHub repository. Here we're setting SK up
# as a statically linked library.
include(FetchContent)
set(SK_BUILD_TESTS       OFF CACHE INTERNAL "")
set(SK_BUILD_SHARED_LIBS ON CACHE INTERNAL "")
set (CMAKE_CXX_STANDARD 20)
FetchContent_Declare(
  StereoKitC
  GIT_REPOSITORY https://github.com/StereoKit/StereoKit.git
  GIT_TAG        v0.3.9
)
FetchContent_MakeAvailable(StereoKitC)
find_package( OpenCV REQUIRED )
find_package( MAVSDK REQUIRED )
find_package( PkgConfig REQUIRED )
find_package( OpenGL REQUIRED )
find_package( X11 REQUIRED )
find_package( CURL REQUIRED )
find_package( ZLIB REQUIRED )
find_package( JPEG REQUIRED )
find_package( PNG REQUIRED )
find_package( nlohmann_json 3.10.5 REQUIRED)
find_library( ICU18n_LIBRARIES icui18n )
find_library( ICU_LIBRARIES icuuc )
find_library( WEBP_LIBRARIES webp )
find_library( UV_LIBRARIES uv )
pkg_check_modules( CAIROMM REQUIRED IMPORTED_TARGET cairomm-1.0 )
include_directories( SYSTEM ${OpenCV_INCLUDE_DIRS} )
include_directories( SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/maplibre-native/platform/default/include )
include_directories( SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/maplibre-native/vendor/unordered_dense/include )
include_directories( SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/maplibre-native/vendor/mapbox-base/extras/args )
include_directories( SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/maplibre-native/vendor/mapbox-base/include )
include_directories( SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/maplibre-native/vendor/mapbox-base/extras/expected-lite/include )
include_directories( SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/maplibre-native/vendor/mapbox-base/deps/geojson-vt-cpp/include )
include_directories( SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/maplibre-native/vendor/mapbox-base/deps/geojson.hpp/include )
include_directories( SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/maplibre-native/vendor/mapbox-base/deps/geometry.hpp/include )
include_directories( SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/maplibre-native/vendor/mapbox-base/deps/jni.hpp/include )
include_directories( SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/maplibre-native/vendor/mapbox-base/deps/optional )
include_directories( SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/maplibre-native/vendor/mapbox-base/deps/pixelmatch-cpp/include )
include_directories( SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/maplibre-native/vendor/mapbox-base/deps/shelf-pack-cpp/include )
include_directories( SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/maplibre-native/vendor/mapbox-base/deps/supercluster.hpp/include )
include_directories( SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/maplibre-native/vendor/mapbox-base/deps/variant/include )
include_directories( SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/maplibre-native/vendor/mapbox-base/deps/cheap-ruler-cpp/include )
include_directories( SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/maplibre-native/vendor/mapbox-base/extras/rapidjson/include )
include_directories( SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/maplibre-native/include )
# Add source files to our target executable
add_executable( SK_FPV
  src/main.cpp
  src/common.cpp
  src/hud.cpp
  src/mavlink_setup.cpp
  src/stabilization.cpp
  src/mapping.cpp
)

# Link to dependencies
target_link_libraries( SK_FPV
  PRIVATE StereoKitC
  PUBLIC ${OpenCV_LIBS}
  MAVSDK::mavsdk
  PkgConfig::CAIROMM
  nlohmann_json::nlohmann_json
  ${CMAKE_CURRENT_SOURCE_DIR}/maplibre-native/build/libmbgl-core.a 
  ${CMAKE_CURRENT_SOURCE_DIR}/maplibre-native/build/libmbgl-vendor-csscolorparser.a 
  ${CMAKE_CURRENT_SOURCE_DIR}/maplibre-native/build/libmbgl-vendor-parsedate.a
  ${OPENGL_LIBRARIES}
  ${X11_LIBRARIES}
  ${CURL_LIBRARIES}
  ${ZLIB_LIBRARIES}
  ${JPEG_LIBRARIES}
  ${PNG_LIBRARIES}
  ${WEBP_LIBRARIES}
  ${ICU18n_LIBRARIES}
  ${ICU_LIBRARIES}
  ${UV_LIBRARIES}
  ${CMAKE_CURRENT_SOURCE_DIR}/maplibre-native/build/libmbgl-vendor-nunicode.a 
  ${CMAKE_CURRENT_SOURCE_DIR}/maplibre-native/build/libmbgl-vendor-sqlite.a 
  
)

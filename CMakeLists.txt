# To use this template, 
cmake_minimum_required(VERSION 3.11)
project(SK_FPV VERSION "0.0.1" LANGUAGES CXX C)

# Grab and build StereoKit from the GitHub repository. Here we're setting SK up
# as a statically linked library.
include(FetchContent)
set(SK_BUILD_TESTS       OFF CACHE INTERNAL "")
set(SK_BUILD_SHARED_LIBS ON CACHE INTERNAL "")
FetchContent_Declare(
  StereoKitC
  GIT_REPOSITORY https://github.com/StereoKit/StereoKit.git
  GIT_TAG        v0.3.9
)
FetchContent_MakeAvailable(StereoKitC)
find_package( OpenCV REQUIRED )
find_package( MAVSDK REQUIRED )
find_package( PkgConfig REQUIRED )
pkg_check_modules( CAIROMM REQUIRED IMPORTED_TARGET cairomm-1.0 )
include_directories( ${OpenCV_INCLUDE_DIRS} )
# Add source files to our target executable
add_executable( SK_FPV
  src/main.cpp
  src/common.cpp
  src/hud.cpp
  src/mavlink_setup.cpp
)

# Link to dependencies
target_link_libraries( SK_FPV
  PRIVATE StereoKitC
  ${OpenCV_LIBS}
  MAVSDK::mavsdk
  PkgConfig::CAIROMM
)

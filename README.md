# FPV VR video player using OpenCV+gstreamer and StereoKit

Compiles and works on Ubuntu 24.10, using GCC and G++ 11.x

Edit your video inputs in "cam.json", which should be in the same folder as the executable.

There is support for single and dual cameras, not stereo surfaces yet. Fisheye corrections are available, you must calibrate your lens and obtain the K, D matrices and the image resolution of the original calibration.

Listens to mavlink at udp://0.0.0.0:14551, you can change the corresponding line in mavlink_setup.cpp to another address or a serial port.

The maps require a "style.json" file to load the map styles, I included a sample style.json with free satellite images. Set up your custom style by changing the API key and json url for the mapping object in main.cpp.

Copy everything in the "assets" folder to the same folder where the binary is.

The headset output is transmitted as a 40mbps intra-only H264 rtp stream on port 7600 and a 4mbps lower quality stream on port 7601.

## Linux pre-requisites

Linux users will need to install some pre-requisites: 

* Monado and your headsets drivers should be working first, for example on my CV1 I also need the OpenHMD libraries.

* OpenCV with gstreamer, Ubuntu 24.10 has it enabled by default

* MAVSDK 2.x, 3.0 is not compatible yet. (https://github.com/mavlink/MAVSDK)
  Set "MAVLINK_DIALECT" in the CMakeLists.txt to "ardupilotmega", since the HUD needs the AoA and SSA messages.

* libcairomm-1.0 for the HUD.

* maplibre-native https://github.com/maplibre/maplibre-native for the live map. Clone it inside this repo, set the commit to 8c6b36811b8f06f12dfc08dbce20ecc7eeca1dfa and build it following the instructions for linux, it should compile the necessary libraries.

* ZMQ C++ and msgpack-c to receive and display the target overlays. 

* Some extra libraries for maplibre, cmake should find them all or show an error telling you what to install.

## Command line instructions

Compiling maplibre-native

```shell
# From the project root directory

#Set C and C++ compilers to gcc-11.x

export CC=gcc-11
export CXX=g++-11

# Download and compile maplibre-native, no need to install it.

git clone https://github.com/maplibre/maplibre-native.git
cd maplibre-native
git checkout 8c6b36811b8f06f12dfc08dbce20ecc7eeca1dfa
git submodule update --init --recursive
cmake -B build -GNinja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DMLN_WITH_COVERAGE=OFF -DCMAKE_BUILD_WITH_INSTALL_RPATH=ON
cmake --build build --target mbgl-render -j $(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null)

cd ..

mkdir build

cd build

cmake ..

#Install any missing libraries if cmake throws an error

make -j 16

cp ../assets/* .

#Edit cam.json and styles.json to your needs.

./SK-FPV

```
## Demo

https://github.com/user-attachments/assets/8dea26b9-9f1d-4348-9e8a-02d015a0dd2e

## TODO

Show ADS-B, geofence, and waypoints in the map.

Listen to RC_CHANNEL messages to control various settings in the app.



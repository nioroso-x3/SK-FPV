# FPV VR video player using OpenCV+gstreamer and StereoKit

Compiles and works on Ubuntu 24.10, using GCC and G++ 11.x

Edit your video inputs in "cam.json", which should be in the same folder as the executable.

There is support for single and dual cameras, not stereo surfaces yet. Fisheye corrections are available, you must calibrate your lens and obtain the K, D matrices and the image resolution of the original calibration.

Listens to mavlink at udp://0.0.0.0:14551, you can change the corresponding line in mavlink_setup.cpp to another address or a serial port.

The maps require a "style.json" file to load the map styles, I included a sample style.json with free satellite images. Set up your custom style by changing the API key and json url for the mapping object in main.cpp.

Copy everything in the "assets" folder to the same folder where the binary is.

The headset output is transmitted as a 40mbps intra-only VP9 rtp stream on port 7600 and a 4mbps lower quality stream on port 7601.

## Linux pre-requisites

Linux users will need to install some pre-requisites for this template to compile. 

* Monado and your headsets drivers should be working first, for example on my CV1 I also need the OpenHMD libraries.

* OpenCV with gstreamer, Ubuntu 24.10 has it enabled by default

* MAVSDK 2.x https://github.com/mavlink/MAVSDK. You will have to comment out the if block in mavsdk_impl.cpp that excludes telemetry radio data for WFB telemetry to work.

* libcairomm-1.0 for the HUD

* maplibre-native https://github.com/maplibre/maplibre-native for the live map. Clone and build it inside this repo following the instructions for linux. USE A COMMIT FROM OCTOBER 2024 TO JUNE 2024! There were some breaking changes.

* msgpack-c to connect to the wfb-ng telemetry.

* Some extra libraries for maplibre, cmake should find them all or show an error telling you what to install.

```shell
sudo apt-get update
sudo apt-get install build-essential cmake unzip libfontconfig1-dev libgl1-mesa-dev libvulkan-dev libx11-xcb-dev libxcb-dri2-0-dev libxcb-glx0-dev libxcb-icccm4-dev libxcb-keysyms1-dev libxcb-randr0-dev libxrandr-dev libxxf86vm-dev mesa-common-dev libjsoncpp-dev libxfixes-dev libglew-dev
```

## Command line instructions

For those new to CMake, here's a quick example of how to compile and build this using the CLI! If something is going wrong, sometimes adding in a `-v` for verbose will give you some additional info you might not see from VS Code.

```shell
# From the project root directory

# Download and compile maplibre-native, no need to install it.

# Make a folder to build in
mkdir build
cd build

# Configure the build
cmake .. 
# Build
make -j 20

# Run the app
./SK_FPV
```
## Demo

https://github.com/user-attachments/assets/6db89d48-5412-468f-955f-6e6380b27d0b

## TODO

Show ADS-B, geofence, and waypoints in the map.

Listen to RC_CHANNEL messages to control various settings in the app.



# FPV VR video player using OpenCV+gstreamer and StereoKit

Compiles and works on Ubuntu 22.04. Tested on Ubuntu 24.04, but it seems OpenCV 4.6 Gstreamer support is broken as VideoCapture objects dont seem to work.

Receives RTP h265 streams on ports 5600 to 5603. You can edit the gstreamer pipelines in main.cpp to change the codecs/ports.

Listens to mavlink at udp://0.0.0.0:14551, you can change the corresponding line in mavlink_setup.cpp to another address or a serial port.

Look at the comments in the code to modify the surface aspect ratio or position of a screen. You can also remove them by deleting them from the main loop.

The maps require a "style.json" file to load the map styles, I included a sample style.json with free satellite images. Set up your custom style by changing the API key and json url for the mapping object in main.cpp.

You also need to move the two png files for the map markers. Just copy everything in the "assets" folder to the same folder where the binary is.

The output is now recorded as 720p30 h265 video saved as "output.ts". The video shows what is displayed in the VR headset. The pipeline is setup in main.cpp right before the renderer loop.

## Linux pre-requisites

Linux users will need to install some pre-requisites for this template to compile. 

* Monado and your headsets drivers should be working first, for example on my CV1 I also need the OpenHMD libraries.

* OpenCV with gstreamer, Ubuntu 22.04 has it enabled by default

* MAVSDK https://github.com/mavlink/MAVSDK. You will have to comment out the if block in mavsdk_impl.cpp that excludes telemetry radio data for WFB telemetry to work.

* libcairomm-1.0 for the HUD

* maplibre-native https://github.com/maplibre/maplibre-native for the live map. Clone and build it inside this repo following the instructions for linux.

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
make -j 4

# Run the app
./SK_FPV
```
## Demo

https://github.com/user-attachments/assets/723a32c0-491a-47d6-b8f6-087a49f1415c

## TODO

Show ADS-B, geofence, and waypoints in the map.

Add RC control using the headset controllers / movement for the plane and mavlink gimbal.



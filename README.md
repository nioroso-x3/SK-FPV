# FPV VR video player using OpenCV+gstreamer and StereoKit

Rough first functional release. My drone has two cameras, so there are two video surfaces.

Compiles and works on Ubuntu 22.04. Tested on Ubuntu 24.04, but it seems OpenCV 4.6 Gstreamer support is broken as VideoCapture objects dont seem to work.

Receives a 16:9 and and two 4:3 RTP h264 streams on ports 5600,5601,5602. You can edit the gstreamer pipelines to change the codecs/ports.

Listens to mavlink at udp://0.0.0.0:14551, change the corresponding line in mavlink_setup.cpp to another address or a serial port.

Look at the comments in the code to modify the surface aspect ratio or position.



## Linux pre-requisites

Linux users will need to install some pre-requisites for this template to compile. 


* Monado and your headsets drivers should be working first, for example on my CV1 I also need the OpenHMD libraries.

* OpenCV with gstreamer, Ubuntu 22.04 has it enabled by default

* MAVSDK https://github.com/mavlink/MAVSDK. You will have to comment out the if block in mavsdk_impl.cpp that excludes telemetry radio data. 

* libcairomm-1.0 for the HUD


```shell
sudo apt-get update
sudo apt-get install build-essential cmake unzip libfontconfig1-dev libgl1-mesa-dev libvulkan-dev libx11-xcb-dev libxcb-dri2-0-dev libxcb-glx0-dev libxcb-icccm4-dev libxcb-keysyms1-dev libxcb-randr0-dev libxrandr-dev libxxf86vm-dev mesa-common-dev libjsoncpp-dev libxfixes-dev libglew-dev
```

## Command line instructions

For those new to CMake, here's a quick example of how to compile and build this using the CLI! If something is going wrong, sometimes adding in a `-v` for verbose will give you some additional info you might not see from VS Code.

```shell
# From the project root directory

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

## TODO

Finish MAVLINK HUD

Add google maps window.

Add stabilization

Add RC control using the headset controllers / movement



# FPV VR video player using OpenCV+gstreamer and StereoKit

Rough first functional release. My drone has two cameras, so there are two video surfaces.

Compiles on Ubuntu 22.04. Follow the instructions for compiling, if you do not set the build type to Debug it wont work.

Receives a 16:9 and a 4:3 RTP h264 streams on ports 5600 and 5601. You can edit the gstreamer pipeline to change the codecs.

Listens to mavlink at port 14540, change the corresponding line in the source to the port that you use

Look at the comments in the code to modify the surface aspect ratio or position.



## Linux pre-requisites

Linux users will need to install some pre-requisites for this template to compile. 

Monado and your headsets drivers should be working first, for example on my CV1 I also need the OpenHMD libraries.

You will also need OpenCV with gstreamer and MAVSDK https://github.com/mavlink/MAVSDK installed and locatable by CMake.


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
cmake .. -DCMAKE_BUILD_TYPE=Debug
# Build
cmake --build . -j8 --config Debug

# Run the app
./SKNativeTemplate
```

## TODO

Finish MAVLINK HUD

Add google maps window.

Add RC control using the headset controllers / movement



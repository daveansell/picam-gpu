cmake_minimum_required(VERSION 2.8)
project( PIRockCV )
find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )

include_directories(/opt/vc/include)
include_directories(/opt/vc/userland-master/interface/vcos)
include_directories(/opt/vc/userland-master)
include_directories(/opt/vc/userland-master/interface/vcos/pthreads)
include_directories(/opt/vc/userland-master/interface/vmcs_host/linux)
include_directories(/opt/vc/userland-master/host_applications/linux/libs/bcm_host/include)
link_directories(/opt/vc/lib)

add_executable( rock-track main.cpp rocks.cpp camera_transforms.cpp camera_constants.cpp ends.cpp kalman.cpp shot.cpp time.cpp gpu.cpp camera.cpp cameracontrol.cpp graphics.cpp lodepng.cpp)
target_link_libraries( rock-track ${OpenCV_LIBS} libmmal_core.so libmmal_util.so libmmal_vc_client.so libvcos.so libbcm_host.so GLESv2 EGL ncurses)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11 -g -fopenmp")


cmake_minimum_required( VERSION 2.8 )
project( pose )

set( CMAKE_BUILD_TYPE Release )
set( CMAKE_CXX_FLAGS "-std=c++11 -O3" )


set(OpenCV_DIR /usr/local/opencv341/share/OpenCV)
find_package( OpenCV )
include_directories( ${OpenCV_INCLUDE_DIRS} )

add_executable( demo1 src/main1.cpp )
target_link_libraries( demo1 ${OpenCV_LIBS})

add_executable( demo2 src/main2.cpp src/PNPSolver.cpp)
target_link_libraries( demo2 ${OpenCV_LIBS})

add_executable( demo3 src/main3.cpp src/PNPSolver.cpp )
target_link_libraries( demo3 ${OpenCV_LIBS})

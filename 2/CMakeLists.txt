cmake_minimum_required(VERSION 2.8)
project( DisplayImage )
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
set(CMAKE_BUILD_TYPE Debug)
find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )
add_executable( test test.cpp )
add_executable( solve solve.cpp )
target_link_libraries( test ${OpenCV_LIBS} )
target_link_libraries( solve ${OpenCV_LIBS} )
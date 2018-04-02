# minimum version of CMake
cmake_minimum_required(VERSION 3.6)

# solution name
project(edgeExtractorOpenCL)

# set executable name
set(TARGET_NAME demoProgram)

# add source code for executable
add_executable(${TARGET_NAME} demoMain.cpp)

# find packages
find_package(OpenCL)
find_package(OpenCV)
# add include dir
include_directories(${OpenCL_INCLUDE_DIRS})
# link with libraries
target_link_libraries(${TARGET_NAME} ${OpenCL_LIBRARIES})
# include path
include_directories(${OpenCV_INCLUDE_DIRS})
# link library path
target_link_libraries(${TARGET_NAME} ${OpenCV_LIBS})


cmake_minimum_required(VERSION 2.6.4)

project(offscreen)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# fixme - only for debug configuration
add_definitions(-D_DEBUG)

find_package(CEF REQUIRED)
find_package(GTK2 REQUIRED)

set(CMAKE_CXX_FLAGS "-g -O0 -std=c++11 -Wall")

set(offscreen_SOURCES
    offscreen.cpp
)

set(offscreen_HEADERS
)

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CEF_INCLUDE_DIRS})
include_directories(${GTK2_INCLUDE_DIRS})

add_executable(offscreen ${offscreen_SOURCES})

target_link_libraries(offscreen ${CEF_LIBRARIES})



cmake_minimum_required(VERSION 2.6.4)

project(nasdaq_trader)

# fixme - only for debug configuration
add_definitions(-D_DEBUG)

find_package(Qt4 4.8 REQUIRED)
include(${QT_USE_FILE})
add_definitions(${QT_DEFINITIONS})

set(CMAKE_CXX_FLAGS "-g -O0 -std=c++11 -Wall")

set(prj1_SOURCES
    prj1.cpp
)

set(prj1_HEADERS
)

qt4_wrap_cpp(prj1_HEADERS_MOC ${prj1_HEADERS})

include_directories(${CMAKE_SOURCE_DIR})

add_executable(prj1 ${prj1_HEADERS_MOC} ${prj1_SOURCES})

target_link_libraries(prj1 ${QT_LIBRARIES})


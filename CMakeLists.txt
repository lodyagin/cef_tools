cmake_minimum_required(VERSION 2.6.4)

project(offscreen)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# fixme - only for debug configuration
add_definitions(-D_DEBUG)

find_package(CEF REQUIRED)
find_package(GTK2 REQUIRED)

set(Boost_USE_MULTITHREADED ON)  
find_package(Boost 1.54 COMPONENTS system REQUIRED)

set(CMAKE_CXX_FLAGS "-g -O0 -std=c++11 -Wall")

add_subdirectory(CUnit/CUnit)
add_subdirectory(PEvents)
add_subdirectory(concurro/C++)

set(offscreen_SOURCES
    browser.cpp
    dom_event.cpp
    offscreen.cpp
    proc_browser.cpp
)

set(offscreen_HEADERS
)

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CEF_INCLUDE_DIRS})
include_directories(${GTK2_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/PEvents)
include_directories(${CMAKE_SOURCE_DIR}/concurro/C++)

add_executable(offscreen ${offscreen_SOURCES})

target_link_libraries(offscreen ${CEF_LIBRARIES})
target_link_libraries(offscreen concurrent)
target_link_libraries(offscreen ${Boost_SYSTEM_LIBRARY})
target_link_libraries(offscreen log4cxx pthread)



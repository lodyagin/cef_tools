cmake_minimum_required(VERSION 2.6.4)

project(offscreen)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# fixme - only for debug configuration
add_definitions(-D_DEBUG)

find_package(CEF REQUIRED)
find_package(GTK2 REQUIRED)

set(Boost_USE_MULTITHREADED ON)  
find_package(Boost 1.54 COMPONENTS system filesystem
  REQUIRED)

set(CMAKE_CXX_FLAGS "-g -O0 -std=c++11 -Wall")

add_subdirectory(CUnit/CUnit)
add_subdirectory(PEvents)
add_subdirectory(concurro/C++)
add_subdirectory(tests)

set(offscreen_SOURCES
    browser.cpp
    dom.cpp
    dom_event.cpp
    ipc.cpp
    offscreen.cpp
    proc_browser.cpp
    screenshotter.cpp
    string_utils.cpp
    task1.cpp
    xpath.cpp
)

set(offscreen_HEADERS
)

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CEF_INCLUDE_DIRS})
include_directories(${GTK2_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/PEvents)
include_directories(${CMAKE_SOURCE_DIR}/concurro/C++)

add_library(offscr STATIC ${offscreen_SOURCES})

target_link_libraries(offscr ${CEF_LIBRARIES})
target_link_libraries(offscr concurrent)
target_link_libraries(offscr ${Boost_SYSTEM_LIBRARY})
target_link_libraries(offscr log4cxx pthread)
target_link_libraries(offscr png)

add_executable(offscreen main.cpp)
target_link_libraries(offscreen offscr)



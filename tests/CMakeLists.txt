add_definitions(-D_DEBUG)

add_subdirectory(gtest)

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CEF_INCLUDE_DIRS})
include_directories(${GTK2_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/PEvents)
include_directories(${CMAKE_SOURCE_DIR}/concurro/C++)
#include_directories(${CEF_INCLUDE_DIRS}/../testing/gtest/include)

add_executable(xpath_test xpath_test.cpp)
add_executable(ipc_test ipc_test.cpp)

target_link_libraries(xpath_test ${CEF_LIBRARIES})
target_link_libraries(xpath_test concurrent)
target_link_libraries(xpath_test ${Boost_SYSTEM_LIBRARY})
target_link_libraries(xpath_test ${Boost_FILESYSTEM_LIBRARY})
target_link_libraries(xpath_test log4cxx pthread)
target_link_libraries(xpath_test gtest)
target_link_libraries(xpath_test offscr)

target_link_libraries(ipc_test ${CEF_LIBRARIES})
target_link_libraries(ipc_test concurrent)
#target_link_libraries(ipc_test ${Boost_SYSTEM_LIBRARY})
#target_link_libraries(ipc_test ${Boost_FILESYSTEM_LIBRARY})
target_link_libraries(ipc_test log4cxx pthread)
target_link_libraries(ipc_test gtest)
target_link_libraries(ipc_test offscr)
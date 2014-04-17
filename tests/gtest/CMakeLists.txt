set(GTEST_DIR /usr/src/gtest) # is standard for Debian

include_directories(${GTEST_DIR})
add_library(gtest STATIC ${GTEST_DIR}/src/gtest-all.cc)
target_link_libraries(gtest log4cxx pthread)

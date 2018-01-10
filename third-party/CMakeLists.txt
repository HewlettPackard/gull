# these are third party code, we don't care warning.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")

set(BUILD_TESTING OFF) # Disable testcases in these third party libraries

add_subdirectory(gtest)

option(BUILD_SHARED_LIBS "Build yaml-cpp as a shared library" ON)
add_subdirectory(yaml-cpp-release-0.5.3)

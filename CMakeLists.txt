cmake_minimum_required(VERSION 2.8.12)
project(nvmm)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

#
# build type: Release (default) or Debug
#
message(STATUS "====================")
if(CMAKE_BUILD_TYPE MATCHES Debug)
  message(STATUS "Build type: Debug")    
elseif(CMAKE_BUILD_TYPE MATCHES Release)
  message(STATUS "Build type: Release")
else()
  message(STATUS "Invalid build type: ${CMAKE_BUILD_TYPE}")
  message(STATUS "Default to : Release")
  set(CMAKE_BUILD_TYPE "Release")    
endif()
message(STATUS "====================")

#
# common gcc flags
#
set(CMAKE_CXX_FLAGS "-std=c++11") 
set(CMAKE_CXX_FLAGS 
  "${CMAKE_CXX_FLAGS} -Wall -W -Wextra -Wredundant-decls -Wunused -Wunused-macros -Wno-unused-parameter -Wcast-qual -Wcast-align -Wwrite-strings -Wmissing-field-initializers -Wendif-labels -Winit-self -Wlogical-op -Wpacked -Wstack-protector -Wformat=2 -Wswitch-enum -Wstrict-overflow=5 -Wpointer-arith -Wnormalized=nfc -Wno-long-long -Wswitch-default -Wconversion -Wunreachable-code")  

# Flags that are currently not used: -Wshadow -Wpadded 
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-invalid-offsetof ") # to suppress warnings about offsetof

#
# gcc flags for release build
#
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS}")

#
# gcc flags for debug build
#
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -DDEBUG -Og")


#
# determine endianness at compile time
#
include(TestBigEndian)
test_big_endian(result)
if(${result})
  add_definitions(-DSYS_BIG_ENDIAN)
  message(STATUS "Current system is big-endian")
else(${result})
  add_definitions(-DSYS_LITTLE_ENDIAN)
  message(STATUS "Current system is little-endian")
endif(${result})

#
# determine tmpfs/lfs location
#
if(LFS) # FAM
  set(TMPFS_PATH "/lfs")
  add_definitions(-DLFS)
  message(STATUS "WARNING: LFS enabled")
  message(STATUS "lfs location: ${TMPFS_PATH}")
else() # SHM
  set(TMPFS_PATH "/dev/shm")
  message(STATUS "tmpfs location: ${TMPFS_PATH}")
endif()

#
# determine shelf base dir. Used to store files backing the shelves
#
if(LFS)
  set(SHELF_BASE_DIR "${TMPFS_PATH}")
else()
  set(SHELF_BASE_DIR "${TMPFS_PATH}/$ENV{USER}")
endif()
add_definitions(-DSHELF_USER="$ENV{USER}")
add_definitions(-DSHELF_BASE_DIR="${SHELF_BASE_DIR}")
message(STATUS "Shelf base dir: ${SHELF_BASE_DIR}")

#
# choose heap implementation
#
option(ZONE "heap implementation" ON)
if(NOT ZONE)
  message(STATUS "WARNING: hierarchical heap")
else()
  set(ZONE ON)
  message(STATUS "WARNING: zone heap")
  add_definitions(-DZONE)
endif()  

#
# default system include and link directories 
#
link_directories(/usr/lib)
include_directories(/usr/include)
link_directories(/usr/local/lib)
include_directories(/usr/local/include)

#
# include and link directories for libfam-atomic and libpmem
#
set(FAMATOMIC_LIB_PATH "fam-atomic path" "/home/$ENV{USER}/lib/libfam-atomic")
link_directories(${FAMATOMIC_LIB_PATH}/lib)
include_directories(${FAMATOMIC_LIB_PATH}/include)

set(PMEM_LIB_PATH "libpmem path" "/home/$ENV{USER}/lib/nvml")
link_directories(${PMEM_LIB_PATH}/lib)
include_directories(${PMEM_LIB_PATH}/include)

#
# add libpmem and lib_fam_atomic
#
set(ARCH_LIBS pmem fam_atomic)

#
# enable GTest
#
enable_testing()
#find_package(GTest REQUIRED)
#include_directories(${GTEST_INCLUDE_DIR})

#
# enable boost
#
find_package(Boost 1.55 COMPONENTS log REQUIRED)

# for boost::log
add_definitions(-DBOOST_LOG_DYN_LINK)


#
# include directories
#
include_directories(${PROJECT_SOURCE_DIR}/include) # public headers (include)
include_directories(${PROJECT_SOURCE_DIR}/src) # private headers (src)
include_directories(${PROJECT_SOURCE_DIR}/test) # test headers (test)
#add_definitions(${ARCH_DEFS})

#
# third-party libraries
#
set(GTEST_ROOT ${PROJECT_SOURCE_DIR}/third-party/gtest)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
add_subdirectory(third-party)
include_directories(${PROJECT_SOURCE_DIR}/third-party/gtest/include)

#
# add source code
#
add_subdirectory(src)

#
# add test code
#

# create base shelf dir
add_custom_target(
  nvmm_shelf_base_dir
  COMMAND ${CMAKE_COMMAND} -E make_directory ${SHELF_BASE_DIR}
  COMMENT create shelf base dir for nvmm
)

add_subdirectory(test)
add_subdirectory(demo)
add_subdirectory(example)



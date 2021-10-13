cmake_minimum_required(VERSION 2.8.11)
project(nvmm)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

#
# default system include and link directories
#
link_directories(/usr/lib)
include_directories(/usr/include)
link_directories(/usr/local/lib)
include_directories(/usr/local/include)

#
# build type: Release (default) or Debug
#
message(STATUS "====================")
if(CMAKE_BUILD_TYPE MATCHES Debug)
  message(STATUS "Build type: Debug")
elseif(CMAKE_BUILD_TYPE MATCHES Release)
  message(STATUS "Build type: Release")
elseif(CMAKE_BUILD_TYPE MATCHES Coverage)
   message(STATUS "Build type: Coverage")
else()
  message(STATUS "Invalid build type: ${CMAKE_BUILD_TYPE}")
  message(STATUS "Default to : Release")
  set(CMAKE_BUILD_TYPE "Release")
endif()

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
if(FAME) # FAME
  add_definitions(-DFAME)
  add_definitions(-DNON_CACHE_COHERENT)
  set(TMPFS_PATH "/lfs")
  message(STATUS "LFS enabled")
  message(STATUS "Lfs location: ${TMPFS_PATH}")
else() # NUMA
  set(TMPFS_PATH "/dev/shm")
  message(STATUS "tmpfs location: ${TMPFS_PATH}")
endif()

#
# There is a bug in current LFS code, which does not mmap from right offset.
# Bug: mmap from any offset incorrectly mmaps from boundaries of Book size
# (8GB default).
#
# So we create header's in case of LFS as the multiple of 8GB (Book size).
# Header extention in case of a Resize also will be multiple of 8GB(Book size).
#
if(LFSWORKAROUND)
  add_definitions(-DLFSWORKAROUND)
  add_definitions(-DNON_CACHE_COHERENT)
endif()

if(LFS_BOOK_SIZE)
  add_definitions(-DLFS_BOOK_SIZE=${LFS_BOOK_SIZE})
endif()

#
# determine shelf base dir. Used to store files backing the shelves
#
if(FAME)
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
  message(STATUS "Heap type: hierarchical")
else()
  set(ZONE ON)
  message(STATUS "Heap type: zone")
  add_definitions(-DZONE)
endif()

#
# enable logging
#
if(LOG)
  message(STATUS "logging: on")
  add_definitions(-DPRINT_LOG)
else()
  message(STATUS "logging: off")
endif()

#
# add libpmem  
#
find_package(Pmem REQUIRED)

set(ARCH_LIBS ${ARCH_LIBS} ${PMEM_LIBRARY})

if(NOT PMEM_HAS_PMEM_INVALIDATE)
  add_definitions(-DPMEM_INVALIDATE_NOOP)
endif()

#
# use libfam_atomic?
#
if(USE_FAM_ATOMIC)
  message(STATUS "Unsupported atomics library option 'FAM', falling back to atomics library: native")
  add_definitions(-DFAM_ATOMIC_NATIVE)
  #find_package(FamAtomic REQUIRED)
  #set(ARCH_LIBS ${ARCH_LIBS} ${FAM_ATOMIC_LIBRARY})
else()
  message(STATUS "atomics library: native")
  add_definitions(-DFAM_ATOMIC_NATIVE)
endif()

#
# add boost
#
set(BOOST_ROOT $ENV{BOOST_ROOT})
find_package(Boost 1.65 COMPONENTS log log_setup thread system filesystem REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  link_directories(${Boost_LIBRARY_DIR})
  #message(STATUS "${Boost_INCLUDE_DIR}")
  #message(STATUS "${Boost_LIBRARY_DIR}")       
endif()


# for boost::log
add_definitions(-DBOOST_LOG_DYN_LINK)

message(STATUS "====================")

#
# common gcc flags
#
set(CMAKE_CXX_FLAGS "-std=c++11")
set(CMAKE_CXX_FLAGS
  "${CMAKE_CXX_FLAGS} -Wall -W -Wextra -Wredundant-decls -Wunused -Wunused-macros -Wno-unused-parameter -Wcast-qual -Wcast-align -Wwrite-strings -Wmissing-field-initializers -Wendif-labels -Winit-self -Wlogical-op -Wpacked -Wstack-protector -Wformat=2 -Wswitch-enum -Wstrict-overflow=5 -Wpointer-arith -Wnormalized=nfc -Wno-long-long -Wconversion -Wunreachable-code")  

# Flags that are currently not used: -Wshadow -Wpadded
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-invalid-offsetof ") # to suppress warnings about offsetof

#
# gcc flags for release build
#
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -g -O2")

#
# gcc flags for debug build
#
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -DDEBUG -Og -g")

set(CMAKE_CXX_FLAGS_COVERAGE "${CMAKE_CXX_FLAGS} -DDEBUG -DCOVERAGE -Og -g -fprofile-arcs -ftest-coverage")

#
# enable GTest
#
enable_testing()

#
# include directories
#
include_directories(${PROJECT_SOURCE_DIR}/include) # public headers (include)
include_directories(${PROJECT_SOURCE_DIR}/src) # private headers (src)
include_directories(${PROJECT_SOURCE_DIR}/test) # test headers (test)

#
# third-party libraries
#
set(GTEST_ROOT ${PROJECT_SOURCE_DIR}/third-party/gtest)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

add_subdirectory(third-party)
include_directories(${PROJECT_SOURCE_DIR}/third-party/gtest/include)
include_directories(${PROJECT_SOURCE_DIR}/third-party/yaml-cpp-release-0.5.3/include)

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

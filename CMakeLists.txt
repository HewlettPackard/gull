cmake_minimum_required(VERSION 3.20)
project(nvmm
  LANGUAGES C CXX)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

#
# build type: Release (default) or Debug
#
message(STATUS "====================")
if(CMAKE_BUILD_TYPE MATCHES Debug)
  message(STATUS "Build type: Debug")
elseif(CMAKE_BUILD_TYPE MATCHES Release)
  message(STATUS "Build type: Release")
elseif(CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
  message(STATUS "Build type: RelWithDebInfo")
elseif(CMAKE_BUILD_TYPE MATCHES Coverage)
  message(STATUS "Build type: Coverage")
else()
  message(STATUS "Invalid build type: ${CMAKE_BUILD_TYPE}")
  message(STATUS "Default to : Release")
  set(CMAKE_BUILD_TYPE "Release")
endif()

#
# determine endianness at compile time, but unused?!
#
add_compile_definitions($<IF:$<STREQUAL:${CMAKE_CXX_BYTE_ORDER},BIG_ENDIAN>,SYS_BIG_ENDIAN,SYS_LITTLE_ENDIAN>)
message(STATUS "Current system is: ${CMAKE_CXX_BYTE_ORDER}")

option(FAME "" OFF)
#
# determine tmpfs/lfs location
#
if(FAME) # FAME
  add_compile_definitions(FAME NON_CACHE_COHERENT)
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
  add_compile_definitions(LFSWORKAROUND NON_CACHE_COHERENT)
endif()

if(LFS_BOOK_SIZE)
  add_compile_definitions(LFS_BOOK_SIZE=${LFS_BOOK_SIZE})
endif()

#
# determine shelf base dir. Used to store files backing the shelves
#

if(FAME)
  set(SHELF_BASE_DIR "${TMPFS_PATH}" CACHE STRING "Shelf base directory")
else()
  set(SHELF_BASE_DIR "${TMPFS_PATH}/$ENV{USER}" CACHE STRING "Shelf base directory")
endif()
set(SHELF_USER "$ENV{USER}" CACHE STRING "Shelf default user")

add_compile_definitions(
  SHELF_USER="${SHELF_USER}"
  SHELF_BASE_DIR="${SHELF_BASE_DIR}")
message(STATUS "Shelf base dir: ${SHELF_BASE_DIR}")
message(STATUS "Shelf default user: ${SHELF_USER}")
#
# choose heap implementation
#
option(ZONE "use Zone heap implementation" ON)
if(NOT ZONE)
  message(STATUS "Heap type: hierarchical")
else()
  message(STATUS "Heap type: zone")
  add_compile_definitions(ZONE)
endif()

#
# enable logging
#
option(LOG "logging" OFF)
if(LOG)
  message(STATUS "Logging: on")
  add_compile_definitions(PRINT_LOG)
else()
  message(STATUS "Logging: off")
endif()

#
# add libpmem
#
find_package(Pmem REQUIRED)

#
# add yaml-cpp
#
find_package(yaml-cpp REQUIRED)

#
# use libfam_atomic?
#
option(USE_FAM_ATOMIC "Use the FamAtomic library for atomics, default: off (use native)" OFF)

if(USE_FAM_ATOMIC)
  message(STATUS "Unsupported atomics library option 'FAM', falling back to atomics library: native")
  add_compile_definitions(FAM_ATOMIC_NATIVE)
  #find_package(FamAtomic REQUIRED)
  #set(ARCH_LIBS ${ARCH_LIBS} ${FAM_ATOMIC_LIBRARY})
else()
  message(STATUS "atomics library: native")
  add_compile_definitions(FAM_ATOMIC_NATIVE)
endif()

#
# add boost
#
find_package(Boost 1.65 COMPONENTS log log_setup thread system filesystem atomic regex REQUIRED)

# for boost::log
add_compile_definitions(BOOST_LOG_DYN_LINK)

message(STATUS "====================")

#
# common gcc flags
#

# while nvmm requires only C++11, gtest needs C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(cxx_flags
  -Wall
  -W
  -Wextra
  -Wredundant-decls
  -Wunused
  -Wunused-macros
  -Wno-unused-parameter
  -Wcast-qual
  -Wcast-align
  -Wwrite-strings
  -Wmissing-field-initializers
  -Wendif-labels
  -Winit-self
  -Wlogical-op
  -Wpacked
  -Wstack-protector
  -Wformat=2
  -Wswitch-enum
  -Wstrict-overflow=5
  -Wpointer-arith
  -Wnormalized=nfc
  -Wno-long-long
  -Wconversion
  -Wunreachable-code
)
set(cxx_debug_flags -g -Og)
set(cxx_coverage_flags ${cxx_debug_flags} -fprofile-arcs -ftest-coverage)
add_compile_options(
  "$<$<COMPILE_LANGUAGE:CXX>:${cxx_flags}>"
  "$<$<CONFIG:Debug>:${cxx_debug_flags}>"
  "$<$<CONFIG:Coverage>:${cxx_coverage_flags}>"
)

add_compile_definitions(
  "$<$<OR:$<CONFIG:Debug>,$<CONFIG:Coverage>>:DEBUG>"
  "$<$<CONFIG:Coverage>:COVERAGE>"
)

include(CheckPIESupported)
check_pie_supported()

# Flags that are currently not used: -Wshadow -Wpadded
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-invalid-offsetof ") # to suppress warnings about offsetof

add_subdirectory(src)

# create base shelf dir, common target for tests and example code
add_custom_target(
  nvmm_shelf_base_dir
  COMMAND ${CMAKE_COMMAND} -E make_directory ${SHELF_BASE_DIR}
  COMMENT create shelf base dir for nvmm
)

option(WITH_EXAMPLES "Build examples and demo code" ON)
if(WITH_EXAMPLES)
  add_subdirectory(demo)
  add_subdirectory(example)
endif()

option(WITH_TESTS "Build tests" ON)
if(WITH_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()

#
# Install targets
#

# the following gives an nvmmTargets.cmake to help other projects directly use the build/ dir
export(TARGETS nvmm NAMESPACE nvmm:: FILE nvmmTargets.cmake)
export(PACKAGE nvmm)

include(GNUInstallDirs)
set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR}  # users of nvmm are supposed to be using: #include <nvmm/...>
    CACHE PATH "Location of header files" )

install(TARGETS nvmm
        EXPORT nvmmTargets
)

# CMake 3.23+ would have FILE_SET instead
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
        DESTINATION "include"
        FILES_MATCHING
        PATTERN "*.h*"
)

include(CMakePackageConfigHelpers)
install(EXPORT nvmmTargets
  FILE nvmmTargets.cmake
  NAMESPACE nvmm::
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/nvmm"
)

configure_package_config_file(cmake/nvmmConfig.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/nvmmConfig.cmake
	INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/nvmm
	PATH_VARS INCLUDE_INSTALL_DIR
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/nvmmConfig.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/nvmm"
)

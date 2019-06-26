cmake_minimum_required(VERSION 3.10)
project(MQSim)

set(CMAKE_CXX_STANDARD 11)

option(FORCE_INLINE "User force inline functions on compile time" On)

# Set the default output path
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

# Append the compile options
# TODO this flags will be open after code cleaning
# set(CMAKE_CXX_FLAGS "-Wall -Werror -Wextra ${CMAKE_CXX_FLAGS}")
# TODO Change optimization flags after code cleaning
set(CMAKE_CXX_FLAGS "-Wall ${CMAKE_CXX_FLAGS}")

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING
      "Choose the type of build, options are: Debug Release Profile." FORCE)
endif (NOT CMAKE_BUILD_TYPE)

if (FORCE_INLINE)
  set(CMAKE_CXX_FLAGS "-DUSE_FORCE_INLINE ${CMAKE_CXX_FLAGS}")
endif (FORCE_INLINE)

# Debug Flags
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CMAKE_CXX_FLAGS "-ggdb3 -O0 -DDEBUG ${CMAKE_CXX_FLAGS}")
elseif (CMAKE_BUILD_TYPE STREQUAL "Profile")
  set(CMAKE_CXX_FLAGS "-ggdb3 -pg -O0 -DPROFILE ${CMAKE_CXX_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")
  set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} -pg")
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -pg")
else (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CMAKE_CXX_FLAGS "-O3 ${CMAKE_CXX_FLAGS}")
endif (CMAKE_BUILD_TYPE STREQUAL "Debug")

add_subdirectory(src)

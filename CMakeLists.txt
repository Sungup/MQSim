cmake_minimum_required(VERSION 3.10)
project(MQSim)

set(CMAKE_CXX_STANDARD 11)

execute_process(
        COMMAND bash -c "git rev-parse --verify --short HEAD"
        OUTPUT_VARIABLE GIT_TAG
)

string(REGEX REPLACE "\n$" "" GIT_TAG ${GIT_TAG})

# ==========================
# Project common information
# ==========================
set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH ${GIT_TAG})
set(CODENAME "refactoring")

# ===============
# Builder options
# ===============
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}-${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_RELEASE 1)

set(CPACK_PACKAGE_NAME "MQSim")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Refactored MQSim for manageable NVMe simulator.")

set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}")

# For rpm package only
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
set(CPACK_RPM_PACKAGE_RELEASE_DIST ON)

# For debian package only
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
set(CPACK_DEBIAN_PACKAGE_SECTION "Electronics")

# Currently serve only RPM package.
set(CPACK_GENERATOR "ZIP;TGZ;TBZ2;RPM;DEB")

# Set package directory to the project home
set(CPACK_OUTPUT_FILE_PREFIX ${PROJECT_SOURCE_DIR})

include(CPack)

# ===============
# Project options
# ===============
option(FORCE_INLINE "User force inline functions on compile time" On)
option(BLOCK_ERASE_HISTO "Gathering block erase histogram" Off)
option(SKIP_EXCEPTION_CHECK "Skip exception status check in logic" On)
option(STATIC_BOOST "Use boost's static libraries" On)

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
  set(CMAKE_CXX_FLAGS "-DUSE_FORCE_INLINE=1 ${CMAKE_CXX_FLAGS}")
endif (FORCE_INLINE)

if (BLOCK_ERASE_HISTO)
  set(CMAKE_CXX_FLAGS "-DGATHER_BLOCK_ERASE_HISTO ${CMAKE_CXX_FLAGS}")
endif (BLOCK_ERASE_HISTO)

if (NOT SKIP_EXCEPTION_CHECK)
  set(CMAKE_CXX_FLAGS "-DRUN_EXCEPTION_CHECK=1 ${CMAKE_CXX_FLAGS}")
endif (NOT SKIP_EXCEPTION_CHECK)

# Debug Flags
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CMAKE_CXX_FLAGS "-ggdb3 -O0 -DDEBUG ${CMAKE_CXX_FLAGS}")
elseif (CMAKE_BUILD_TYPE STREQUAL "Profile")
  # Remove -pg flags becuase of Mojave
  set(CMAKE_CXX_FLAGS "-ggdb3 -O3 -DPROFILE ${CMAKE_CXX_FLAGS}")
  # set(CMAKE_CXX_FLAGS "-ggdb3 -pg -O0 -DPROFILE ${CMAKE_CXX_FLAGS}")
  # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
  # set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")
  # set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} -pg")
  # set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -pg")
else (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CMAKE_CXX_FLAGS "-O3 ${CMAKE_CXX_FLAGS}")
endif (CMAKE_BUILD_TYPE STREQUAL "Debug")

add_subdirectory(src)

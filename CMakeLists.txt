cmake_minimum_required(VERSION 3.10)
project(MQSim)

set(CMAKE_CXX_STANDARD 11)

# Set the default output path
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

# Append the compile options
# TODO this flags will be open after code cleaning
# set(CMAKE_CXX_FLAGS "-Wall -Werror -Wextra ${CMAKE_CXX_FLAGS}")
# set(CMAKE_C_FLAGS   "-Wall -Werror -Wextra ${CMAKE_C_FLAGS}")

# TODO Change optimization flags after code cleaning
set(CMAKE_CXX_FLAGS_RELEASE "-O3 ${CMAKE_CXX_FLAGS_RELEASE}")
set(CMAKE_C_FLAGS_RELEASE "-O3 ${CMAKE_C_FLAGS_RELEASE}")

set(CMAKE_CXX_FLAGS "-Wall ${CMAKE_CXX_FLAGS}")
set(CMAKE_C_FLAGS   "-Wall ${CMAKE_C_FLAGS}")

set(CMAKE_CXX_FLAGS "-DFORCE_INLINE ${CMAKE_CXX_FLAGS}")
set(CMAKE_C_FLAGS "-DFORCE_INLINE ${CMAKE_C_FLAGS}")

add_subdirectory(src)

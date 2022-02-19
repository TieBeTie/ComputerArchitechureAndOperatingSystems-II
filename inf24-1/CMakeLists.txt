cmake_minimum_required(VERSION 3.16.3)
project(inf24_1)

set(CMAKE_CXX_STANDARD 20)

project(fuse-example)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize=leak -g")

find_package(PkgConfig REQUIRED)
pkg_check_modules(FUSE REQUIRED fuse3)

include_directories(${FUSE_INCLUDE_DIRS})
add_executable(fuse-example main.cpp)
target_link_libraries(fuse-example ${FUSE_LIBRARIES})
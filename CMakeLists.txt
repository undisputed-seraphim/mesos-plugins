cmake_minimum_required(VERSION 3.9.6)
project(mesos-plugins VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Mesos REQUIRED)
find_package(Protobuf 3.5.0 REQUIRED)

add_subdirectory("${CMAKE_SOURCE_DIR}/src")
cmake_minimum_required(VERSION 3.10.0)
project(platformer VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum OS X deployment version" FORCE)

add_subdirectory(engine)


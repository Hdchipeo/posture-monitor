# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

# Project Firmware Version (Semantic Versioning: Major.Minor.Patch)
set(PROJECT_VER "1.1.2")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(posture-monitor VERSION 1.1.2)

cmake_minimum_required(VERSION 3.5)

if (POLICY CMP00091)
  # Enable MSVC Runtime Library Property
  cmake_policy(SET CMP0091 NEW)
endif()

project(D3D12Apps LANGUAGES CXX)

add_subdirectory(${PROJECT_SOURCE_DIR}/RotatingPyramid/)
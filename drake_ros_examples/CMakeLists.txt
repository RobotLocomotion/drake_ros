cmake_minimum_required(VERSION 3.10)
project(drake_ros_examples)

# Default to C++17
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

find_package(ament_cmake_ros REQUIRED)
find_package(drake REQUIRED)
find_package(drake_ros_core REQUIRED)
find_package(drake_ros_viz REQUIRED)

add_subdirectory(examples/iiwa_manipulator)
add_subdirectory(examples/multirobot)
add_subdirectory(examples/rs_flip_flop)

if(BUILD_TESTING)
  find_package(ament_cmake_clang_format REQUIRED)
  find_package(ament_cmake_cpplint REQUIRED)

  ament_clang_format(CONFIG_FILE .clang-format)
  ament_cpplint()
endif()

ament_package()

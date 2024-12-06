cmake_minimum_required(VERSION 3.10)
project(drake_ros)

# Default to C++20
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 20)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Werror -Wno-error=deprecated-declarations)
endif()

find_package(ament_cmake_ros REQUIRED)

find_package(drake REQUIRED)
find_package(Python3 COMPONENTS Interpreter Development)
# Must use Drake's fork of Pybind11
find_package(pybind11 REQUIRED HINTS "${drake_DIR}/../pybind11" NO_DEFAULT_PATH)

find_package(eigen3_cmake_module REQUIRED)
find_package(Eigen3)
find_package(rclcpp REQUIRED)
find_package(rosidl_runtime_c REQUIRED)
find_package(rosidl_typesupport_cpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(tf2_eigen REQUIRED)
find_package(visualization_msgs REQUIRED)

add_subdirectory(core)
add_subdirectory(tf2)
add_subdirectory(viz)
# Python bindings
add_subdirectory(drake_ros)

# Ament installation must happen in top-level, including Python package
# installation.
# TODO(sloretz) exclude drake_ros/test folder
ament_python_install_package(drake_ros)

# Generate same python package in build directory for use in tests to allow
# for non-install-space testing of this package. For more information, see
#https://github.com/ros2/ros2_documentation/issues/3373
file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/py/")
file(COPY "drake_ros"
  DESTINATION "${PROJECT_BINARY_DIR}/py/"
  PATTERN "drake_ros/test" EXCLUDE)

if(BUILD_TESTING)
    find_package(ament_cmake_clang_format REQUIRED)
    find_package(ament_cmake_cpplint REQUIRED)
    find_package(ament_cmake_pycodestyle REQUIRED)

    ament_clang_format(CONFIG_FILE .clang-format)
    ament_cpplint()
    # TODO(Aditya) : Remove the --exclude once https://github.com/ament/ament_lint/pull/435
    # is released into ROS Humble.
    ament_pycodestyle(--config pycodestyle.ini --exclude
      "${CMAKE_CURRENT_SOURCE_DIR}/drake_ros/test")
endif()

### Drake library path environment hook
get_target_property(DRAKE_LIBRARY drake::drake LOCATION)
get_filename_component(DRAKE_LIBRARY_DIR ${DRAKE_LIBRARY} DIRECTORY)
if(APPLE)
  set(LIBRARY_PATH_ENV_VAR "DYLD_LIBRARY_PATH")
else()
  set(LIBRARY_PATH_ENV_VAR "LD_LIBRARY_PATH")
endif()
set(
  AMENT_CMAKE_ENVIRONMENT_HOOKS_DESC_drake_library_path
  "prepend-non-duplicate;${LIBRARY_PATH_ENV_VAR};${DRAKE_LIBRARY_DIR}")
ament_environment_hooks("env-hooks/drake_library_path.sh.in")

### Python module path environment hook for pydrake
set(
  AMENT_CMAKE_ENVIRONMENT_HOOKS_DESC_pydrake_pythonpath
  "prepend-non-duplicate;PYTHONPATH;${drake_PYTHON_DIR}")
ament_environment_hooks("env-hooks/pydrake_pythonpath.sh.in")


ament_export_targets(${PROJECT_NAME} HAS_LIBRARY_TARGET)

ament_export_dependencies(drake)
ament_export_dependencies(eigen3_cmake_module)
ament_export_dependencies(Eigen3)
ament_export_dependencies(geometry_msgs)
ament_export_dependencies(rclcpp)
ament_export_dependencies(rosidl_runtime_c)
ament_export_dependencies(rosidl_typesupport_cpp)
ament_export_dependencies(sensor_msgs)
ament_export_dependencies(tf2_eigen)
ament_export_dependencies(tf2_ros)
ament_export_dependencies(visualization_msgs)

ament_package()

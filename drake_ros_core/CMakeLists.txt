cmake_minimum_required(VERSION 3.10)
project(drake_ros_core)

# Default to C++17
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

find_package(ament_cmake_ros REQUIRED)
find_package(drake REQUIRED)
# Must use Drake's fork of Pybind11
find_package(pybind11 REQUIRED HINTS "${drake_DIR}/../pybind11" NO_DEFAULT_PATH)
find_package(rclcpp REQUIRED)
find_package(rosidl_runtime_c REQUIRED)
find_package(rosidl_typesupport_cpp REQUIRED)

add_library(drake_ros_core
  src/drake_ros.cc
  src/publisher.cc
  src/ros_interface_system.cc
  src/ros_publisher_system.cc
  src/ros_subscriber_system.cc
  src/subscription.cc
)

target_link_libraries(drake_ros_core PUBLIC
    drake::drake
    rclcpp::rclcpp
    rosidl_runtime_c::rosidl_runtime_c
    rosidl_typesupport_cpp::rosidl_typesupport_cpp
)

target_include_directories(drake_ros_core
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>"
    "$<INSTALL_INTERFACE:include>"
)

ament_export_include_directories(include)
ament_export_libraries(drake_ros_core)
ament_export_targets(${PROJECT_NAME})

ament_export_dependencies(drake)
ament_export_dependencies(rclcpp)
ament_export_dependencies(rosidl_runtime_c)
ament_export_dependencies(rosidl_typesupport_cpp)

install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(
  DIRECTORY include/
  DESTINATION include
)

ament_python_install_package(
  drake_ros_core PACKAGE_DIR src/drake_ros_core)

###
# Python bindings
###
pybind11_add_module(drake_ros_core_py SHARED
  src/python_bindings/drake_ros_core.cc
)
target_link_libraries(drake_ros_core_py PRIVATE drake_ros_core)
set_target_properties(drake_ros_core_py PROPERTIES OUTPUT_NAME "_drake_ros_core")
target_include_directories(drake_ros_core_py
  PRIVATE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/python_bindings>"
)

ament_get_python_install_dir(python_install_dir)

install(
  TARGETS drake_ros_core_py
  DESTINATION "${python_install_dir}"
)
### End Python bindings

if(BUILD_TESTING)
  find_package(ament_cmake_clang_format REQUIRED)
  find_package(ament_cmake_cpplint REQUIRED)
  find_package(ament_cmake_gtest REQUIRED)
  find_package(ament_cmake_pytest REQUIRED)
  find_package(ament_cmake_pycodestyle REQUIRED)
  find_package(test_msgs REQUIRED)

  ament_clang_format(CONFIG_FILE .clang-format)
  ament_cpplint()

  ament_pycodestyle(--config pycodestyle.ini)

  ament_add_gtest(test_pub_sub test/test_pub_sub.cc)

  target_compile_definitions(test_pub_sub
    PRIVATE
    # We do not expose `rmw_isoliation` via CMake.
    _TEST_DISABLE_RMW_ISOLATION
  )
  target_link_libraries(test_pub_sub
    drake::drake
    drake_ros_core
    ${test_msgs_TARGETS}
  )

  # TODO(eric, shane): Add environment variable to disable rmw_isolation.
  ament_add_pytest_test(test_pub_sub_py test/test_pub_sub.py)

  ament_add_gtest(test_drake_ros test/test_drake_ros.cc)
  target_compile_definitions(test_drake_ros
    PRIVATE
    # We do not expose `rmw_isoliation` via CMake.
    _TEST_DISABLE_RMW_ISOLATION
  )
  target_link_libraries(test_drake_ros
    drake_ros_core
  )
endif()

ament_package()

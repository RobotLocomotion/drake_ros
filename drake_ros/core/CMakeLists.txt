set(HEADERS
  "camera_info_system.h"
  "clock_system.h"
  "drake_ros.h"
  "geometry_conversions.h"
  "geometry_conversions_pybind.h"
  "ros_idl_pybind.h"
  "publisher.h"
  "image_system.h"
  "ros_interface_system.h"
  "ros_publisher_system.h"
  "ros_subscriber_system.h"
  "serializer.h"
  "serializer_interface.h"
)

# Mock install headers so include paths match installed paths
set(mock_include_dir "${CMAKE_CURRENT_BINARY_DIR}/include")
file(MAKE_DIRECTORY "${mock_include_dir}/drake_ros/core")
foreach(hdr ${HEADERS})
  configure_file("${hdr}" "${mock_include_dir}/drake_ros/core/${hdr}" COPYONLY)
endforeach()

add_library(drake_ros_core
  camera_info_system.cc
  clock_system.cc
  drake_ros.cc
  geometry_conversions.cc
  publisher.cc
  image_system.cc
  ros_interface_system.cc
  ros_publisher_system.cc
  ros_subscriber_system.cc
  subscription.cc
)

target_link_libraries(drake_ros_core PUBLIC
  Eigen3::Eigen
  drake::drake
  ${geometry_msgs_TARGETS}
  rclcpp::rclcpp
  rosidl_runtime_c::rosidl_runtime_c
  rosidl_typesupport_cpp::rosidl_typesupport_cpp
  ${sensor_msgs_TARGETS}
)

target_include_directories(drake_ros_core
  PUBLIC
    "$<BUILD_INTERFACE:${mock_include_dir}>"
    "$<INSTALL_INTERFACE:include>"
)

# Nominal ament install directives
install(TARGETS drake_ros_core EXPORT ${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(
  FILES
    ${HEADERS}
  DESTINATION include/drake_ros/core
)

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  find_package(test_msgs REQUIRED)

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

  ament_add_gtest(test_drake_ros test/test_drake_ros.cc)
  target_compile_definitions(test_drake_ros
    PRIVATE
    # We do not expose `rmw_isoliation` via CMake.
    _TEST_DISABLE_RMW_ISOLATION
  )
  target_link_libraries(test_drake_ros
    drake_ros_core
  )

  ament_add_gtest(test_geometry_conversions test/test_geometry_conversions.cc)
  target_link_libraries(test_geometry_conversions drake_ros_core)


  ament_add_gtest(test_rgbd_system test/test_rgbd_system.cc)
  target_compile_definitions(test_rgbd_system
    PRIVATE
    # We do not expose `rmw_isoliation` via CMake.
    _TEST_DISABLE_RMW_ISOLATION
  )
  target_link_libraries(test_rgbd_system
    drake_ros_core
  )

  ament_add_gtest(test_clock_system test/test_clock_system.cc)
  target_compile_definitions(test_clock_system
    PRIVATE
    # We do not expose `rmw_isoliation` via CMake.
    _TEST_DISABLE_RMW_ISOLATION
  )
  target_link_libraries(test_clock_system
    drake_ros_core
  )

  ament_add_gtest(test_camera_info_system test/test_camera_info_system.cc)
  target_compile_definitions(test_camera_info_system
    PRIVATE
    # We do not expose `rmw_isoliation` via CMake.
    _TEST_DISABLE_RMW_ISOLATION
  )
  target_link_libraries(test_camera_info_system
    drake_ros_core
  )
endif()

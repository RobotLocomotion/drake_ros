add_executable(rgbd rgbd.cc)
target_link_libraries(rgbd
  ament_index_cpp::ament_index_cpp
  drake::drake
  drake_ros::drake_ros_core
  drake_ros::drake_ros_viz
  gflags_shared
)

install(
  TARGETS
  rgbd
  DESTINATION lib/${PROJECT_NAME}
)

install(FILES
  rgbd.sdf
  rgbd.rviz
  DESTINATION share/${PROJECT_NAME}/rgbd/
)

install(
  PROGRAMS rgbd.py
  DESTINATION lib/${PROJECT_NAME}
)

install(
  DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}
  FILES_MATCHING PATTERN "*_launch.py"
)

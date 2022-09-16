add_executable(multirobot multirobot.cc)
target_link_libraries(multirobot
  drake::drake
  drake_ros_core::drake_ros_core
  drake_ros_tf2::drake_ros_tf2
  drake_ros_viz::drake_ros_viz
)

install(
  TARGETS multirobot
  DESTINATION lib/${PROJECT_NAME}
)

install(
  PROGRAMS multirobot.py
  DESTINATION lib/${PROJECT_NAME}
)

install(
  FILES multirobot.rviz
  DESTINATION share/${PROJECT_NAME}
)

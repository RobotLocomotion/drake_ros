add_executable(iiwa_manipulator iiwa_manipulator.cpp)
target_link_libraries(iiwa_manipulator
  drake::drake
  drake_ros_core::drake_ros_core
  drake_ros_viz::drake_ros_viz
)

install(
  PROGRAMS iiwa_manipulator.py
  DESTINATION lib/${PROJECT_NAME}
)

install(
  TARGETS
    iiwa_manipulator
  DESTINATION lib/${PROJECT_NAME}
)

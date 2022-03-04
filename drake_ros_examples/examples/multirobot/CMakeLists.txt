install(
  PROGRAMS multirobot.py
  DESTINATION lib/${PROJECT_NAME}
)

install(
  DIRECTORY iiwa_description
  DESTINATION share/${PROJECT_NAME}
)

install(
  FILES multirobot.rviz
  DESTINATION share/${PROJECT_NAME}
)

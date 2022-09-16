AVAILABLE_TYPESUPPORT_LIST = @AVAILABLE_TYPESUPPORT_LIST@

REPOSITORY_ROOT = @REPOSITORY_ROOT@

# Prepend full paths to not break workspace overlays
RUNTIME_ENVIRONMENT = {
  "AMENT_PREFIX_PATH": ["path-prepend"] + @AMENT_PREFIX_PATHS@,
  "${LOAD_PATH}": ["path-prepend"] + @LOAD_PATHS@,
  "ROS_LOCALHOST_ONLY": ["set-if-not-set", @DEFAULT_LOCALHOST_ONLY@],
}

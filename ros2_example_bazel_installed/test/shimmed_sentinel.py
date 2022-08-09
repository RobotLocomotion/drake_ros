import os


assert __name__ == '__main__'

shimmed = 'yes' if '_BAZEL_ROS2_RULES_SHIMMED' in os.environ else 'no'
app_present = 'yes' if 'AMENT_PREFIX_PATH' in os.environ else 'no'

print(f'shimmed: {shimmed} AMENT_PREFIX_PATH present: {app_present}')

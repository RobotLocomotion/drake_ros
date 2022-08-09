import json
import os


assert __name__ == '__main__'

result = {
    'shimmed': '_BAZEL_ROS2_RULES_SHIMMED' in os.environ,
    'AMENT_PREFIX_PATH present': 'AMENT_PREFIX_PATH' in os.environ
}

print(json.dumps(result))

import functools
import json
import os

import ros2bzl.sandboxing as sandboxing

PATH_TO_RESOURCES = os.path.realpath(
    os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
)


def path_to_resource(name):
    return os.path.join(PATH_TO_RESOURCES, name)


@functools.lru_cache(maxsize=None)
def load_resource(name):
    with open(path_to_resource(name), 'r') as fd:
        return fd.read()

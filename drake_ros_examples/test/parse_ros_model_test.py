"""
Shows how to locate and parse a simple model from the ROS 2 ament index.

For more details on the ament index, please see:
https://github.com/ament/ament_cmake/blob/master/ament_cmake_core/doc/resource_index.md
"""  # noqa

import sys

import pytest

from pydrake.multibody.parsing import Parser
from pydrake.multibody.plant import MultibodyPlant


def test_parse_model():
    plant = MultibodyPlant(time_step=0.0)
    parser = Parser(plant)
    # TODO(eric.cousineau): Incorporate `AMENT_PREFIX_PATH` into
    # PackageMap::PopulateFromRosPackagePath().
    parser.package_map().PopulateFromEnvironment("AMENT_PREFIX_PATH")
    # TODO(eric.cousineau): Use a non-test resource.
    parser.AddModelsFromUrl(
        "package://rviz_default_plugins/test_meshes/test.urdf"
    )
    plant.Finalize()


if __name__ == '__main__':
    sys.exit(pytest.main(sys.argv))

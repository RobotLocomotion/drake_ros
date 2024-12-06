"""Python wrapper for drake_ros.core."""

from drake_ros._cc.core import CameraInfoSystem
from drake_ros._cc.core import ClockSystem
from drake_ros._cc.core import init
from drake_ros._cc.core import CppNode
from drake_ros._cc.core import CppNodeOptions
from drake_ros._cc.core import DrakeRos
from drake_ros._cc.core import Isometry3ToRosPose
from drake_ros._cc.core import Isometry3ToRosTransform
from drake_ros._cc.core import QuaternionToRosQuaternion
from drake_ros._cc.core import ImageSystem
from drake_ros._cc.core import RigidTransformToRosPose
from drake_ros._cc.core import RigidTransformToRosTransform
from drake_ros._cc.core import RosAccelToSpatialAcceleration
from drake_ros._cc.core import RosAccelToVector6
from drake_ros._cc.core import RosInterfaceSystem
from drake_ros._cc.core import RosPointToVector3
from drake_ros._cc.core import RosPoseToIsometry3
from drake_ros._cc.core import RosPoseToRigidTransform
from drake_ros._cc.core import RosPublisherSystem
from drake_ros._cc.core import RosQuaternionToQuaternion
from drake_ros._cc.core import RosQuaternionToRotationMatrix
from drake_ros._cc.core import RosSubscriberSystem
from drake_ros._cc.core import RosTransformToIsometry3
from drake_ros._cc.core import RosTransformToRigidTransform
from drake_ros._cc.core import RosTwistToSpatialVelocity
from drake_ros._cc.core import RosTwistToVector6
from drake_ros._cc.core import RosVector3ToVector3
from drake_ros._cc.core import RosWrenchToSpatialForce
from drake_ros._cc.core import RosWrenchToVector6
from drake_ros._cc.core import RotationMatrixToRosQuaternion
from drake_ros._cc.core import SerializerInterface
from drake_ros._cc.core import SpatialAccelerationToRosAccel
from drake_ros._cc.core import SpatialForceToRosWrench
from drake_ros._cc.core import SpatialVelocityToRosTwist
from drake_ros._cc.core import Vector3ToRosPoint
from drake_ros._cc.core import Vector3ToRosVector3
from drake_ros._cc.core import Vector6ToRosAccel
from drake_ros._cc.core import Vector6ToRosTwist
from drake_ros._cc.core import Vector6ToRosWrench
from drake_ros._cc.core import shutdown

from pydrake.common.value import AbstractValue
from pydrake.systems.framework import TriggerType

from rclpy.serialization import serialize_message
from rclpy.serialization import deserialize_message
from rclpy.type_support import check_for_type_support


def _setattr_kwargs(obj, kwargs):
    # For `ParamInit` in `drake_ros_pybind.h`.
    for name, value in kwargs.items():
        setattr(obj, name, value)


class PySerializer(SerializerInterface):
    """
    A (de)serialization interface for Python ROS messages.
    """

    def __init__(self, message_type):
        SerializerInterface.__init__(self)
        check_for_type_support(message_type)
        self._message_type = message_type

    def GetTypeSupport(self):
        return self._message_type._TYPE_SUPPORT

    def CreateDefaultValue(self):
        return AbstractValue.Make(self._message_type())

    def Serialize(self, abstract_value):
        return serialize_message(abstract_value.get_value())

    def Deserialize(self, serialized_message, abstract_value):
        abstract_value.set_value(deserialize_message(
            serialized_message, self._message_type))


@staticmethod
def _make_ros_publisher_system(
    message_type, topic_name, qos, ros_interface,
    publish_triggers={
        TriggerType.kPerStep,
        TriggerType.kForced},
    publish_period=0.0
):
    return RosPublisherSystem(
        PySerializer(message_type),
        topic_name, qos, ros_interface,
        publish_triggers, publish_period)


RosPublisherSystem.Make = _make_ros_publisher_system


@staticmethod
def _make_ros_subscriber_system(
    message_type, topic_name, qos, ros_interface
):
    return RosSubscriberSystem(
        PySerializer(message_type),
        topic_name, qos, ros_interface)


RosSubscriberSystem.Make = _make_ros_subscriber_system


__all__ = [
    'CameraInfoSystem',
    'ClockSystem',
    'DrakeRosInterface',
    'Isometry3ToRosPose',
    'Isometry3ToRosTransform',
    'PySerializer',
    'QuaternionToRosQuaternion',
    'ImageSystem',
    'RigidTransformToRosPose',
    'RigidTransformToRosTransform',
    'RosAccelToSpatialAcceleration',
    'RosAccelToVector6',
    'RosInterfaceSystem',
    'RosPointToVector3',
    'RosPoseToIsometry3',
    'RosPoseToRigidTransform',
    'RosPublisherSystem',
    'RosQuaternionToQuaternion',
    'RosQuaternionToRotationMatrix',
    'RosSubscriberSystem',
    'RosTransformToIsometry3',
    'RosTransformToRigidTransform',
    'RosTwistToSpatialVelocity',
    'RosTwistToVector6',
    'RosVector3ToVector3',
    'RosWrenchToSpatialForce',
    'RosWrenchToVector6',
    'RotationMatrixToRosQuaternion',
    'SerializerInterface',
    'SpatialAccelerationToRosAccel',
    'SpatialForceToRosWrench',
    'SpatialVelocityToRosTwist',
    'Vector3ToRosPoint',
    'Vector3ToRosVector3',
    'Vector6ToRosAccel',
    'Vector6ToRosTwist',
    'Vector6ToRosWrench',
    'init',
    'shutdown',
]

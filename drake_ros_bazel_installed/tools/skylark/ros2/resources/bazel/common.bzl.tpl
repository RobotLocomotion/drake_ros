# -*- python -*-

load(
    "@drake_ros//tools/skylark/ros2:rmw.bzl",
    "use_fastrtps_profile",
)

def incorporate_rmw_implementation(kwargs, env_changes, rmw_implementation):
    target = "@REPOSITORY_ROOT@:%s_cc" % rmw_implementation
    kwargs["data"] = kwargs.get("data", []) + [target]
    env_changes = dict(env_changes)
    env_changes.update({
         "RMW_IMPLEMENTATION": ["replace", rmw_implementation]
    })
    return kwargs, env_changes

def incorporate_fastrtps_profile(kwargs, env_changes, profile_name):        
    kwargs["data"] = kwargs.get("data", []) + [profile_name]
    env_changes = dict(env_changes)
    env_changes.update(use_fastrtps_profile(profile_name))
    return kwargs, env_changes
# -*- python -*-

_COMMON_KWARGS = [
    "compatible_with",
    "deprecation",
    "exec_compatible_with",
    "exec_properties",
    "features",
    "restricted_to",
    "tags",
    "target_compatible_with",
    "testonly",
    "toolchains",
    "visibility",
]

def filter_to_only_common_kwargs(kwargs):
    """Fetch keyword arguments common to all rules from `kwargs`."""
    return {key: value for key, value in kwargs.items() if key in _COMMON_KWARGS}

_TEST_KWARGS = [
    "env_inherit",
    "flaky",
    "local",
    "shard_count",
    "size",
    "timeout",
]

def remove_test_specific_kwargs(kwargs):
    """Filter keyword arguments specific to test rules from `kwargs`."""
    return {key: value for key, value in kwargs.items() if key not in _TEST_KWARGS}

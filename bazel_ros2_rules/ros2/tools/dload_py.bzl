# -*- python -*-
# vi: set ft=python :

"""
The purpose of these rules is to support the propagation of runtime information
that is necessary for the execution of Python binaries and tests that require it.
"""

load(
    "//tools:dload.bzl",
    "do_dload_shim",
    "get_dload_shim_attributes",
)

# TODO(eric.cousineau): We should ideally separate out as much as this actual
# logic into separate library, and make entry point minimal.
_DLOAD_PY_SHIM_TEMPLATE = """\
import os
import sys

from bazel_tools.tools.python.runfiles import runfiles

SHIMMED_SENTINEL = "_BAZEL_ROS2_RULES_SHIMMED";


def main(argv):
    r = runfiles.Create()
    # NOTE(hidmic): unlike its C++ equivalent, Python runfiles'
    # builtin tools will only look for runfiles in the manifest
    # if there is a manifest
    runfiles_dir = r.EnvVars()['RUNFILES_DIR']

    def rlocation(path):
        return r.Rlocation(path) or os.path.join(runfiles_dir, path)

    if SHIMMED_SENTINEL not in os.environ:
        for name, action in zip({names}, {actions}):  # noqa
            action_type, action_args = action[0], action[1:]
            if action_type == 'replace':
                assert len(action_args) == 1
                value = action_args[0]
            elif action_type == 'set-if-not-set':
                assert len(action_args) == 1
                if name in os.environ:
                    continue
                value = action_args[0]
            elif action_type == 'path-replace':
                assert len(action_args) == 1
                value = rlocation(action_args[0])
            elif action_type == 'path-prepend':
                assert len(action_args) > 0
                value = ':'.join([rlocation(path) for path in action_args])
                if name in os.environ:
                    value += ':' + os.environ[name]
            else:
                assert False  # should never get here
            if '$PWD' in value:
                value = value.replace('$PWD', os.getcwd())
            os.environ[name] = value
        os.environ[SHIMMED_SENTINEL] = ""

    executable_path = r.Rlocation('{executable_path}')  # noqa
    argv = [executable_path] + argv[1:]
    os.execv(executable_path, argv)


if __name__ == '__main__':
    main(sys.argv)
"""

def _to_py_list(collection):
    """Turn collection into a Python list expression."""
    return "[" + ", ".join(collection) + "]"

def _dload_py_shim_impl(ctx):
    return do_dload_shim(ctx, _DLOAD_PY_SHIM_TEMPLATE, _to_py_list)

dload_py_shim = rule(
    attrs = get_dload_shim_attributes(),
    output_to_genfiles = True,
    implementation = _dload_py_shim_impl,
)
"""
Generates a Python shim that can inject runtime environment information for
Python binaries that have such requirements. Using a Python shim for Python
binaries enables downstream usage of the latter through transitive dependencies.

See do_dload_shim() documentation for further reference.
"""

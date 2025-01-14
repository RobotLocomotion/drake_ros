# -*- python -*-
# vi: set ft=python :

"""
The purpose of these rules is to support the propagation of runtime information
that is necessary for the execution of Python binaries and tests that require
it.
"""

load(
    "//tools:dload.bzl",
    "do_dload_shim",
    "get_dload_shim_attributes",
)

_ISOLATE_IMPORT = """\
import network_isolation.network_isolation_py as isolation
"""

_ISOLATE_CALL_OR_RETURN = """\
if not isolation.create_linux_namespaces():
    sys.exit(-1)
"""

_DLOAD_PY_SHIM_TEMPLATE = """\
assert __name__ == "__main__"

from bazel_ros2_rules.ros2.tools.dload_shim import do_dload_shim
PY_ISOLATE_IMPORT

PY_ISOLATE_CALL
executable_path = "{executable_path}"
names = {names}
actions = {actions}
do_dload_shim(executable_path, names, actions)
"""

def _resolve_isolation(template, network_isolation):
    isolate_import = ""
    isolate_call = ""
    if network_isolation:
        isolate_import = _ISOLATE_IMPORT
        isolate_call = _ISOLATE_CALL_OR_RETURN
    template = template.replace("PY_ISOLATE_IMPORT", isolate_import)
    template = template.replace("PY_ISOLATE_CALL", isolate_call)
    return template

def _to_py_list(collection):
    """Turn collection into a Python list expression."""
    return "[" + ", ".join(collection) + "]"

def _dload_py_shim_impl(ctx):
    template = _resolve_isolation(
        _DLOAD_PY_SHIM_TEMPLATE,
        ctx.attr.network_isolation,
    )
    return do_dload_shim(ctx, template, _to_py_list)

dload_py_shim = rule(
    attrs = get_dload_shim_attributes(),
    output_to_genfiles = True,
    implementation = _dload_py_shim_impl,
)
"""
Generates a Python shim that can inject runtime environment information for
Python binaries that have such requirements. Using a Python shim for Python
binaries enables downstream usage of the latter through transitive
dependencies.

This shim uses a sentinel environment variable so that it only modifies the
environment once. Any nested shims will use the top-level shim's environment.

See do_dload_shim() documentation for further reference.
"""

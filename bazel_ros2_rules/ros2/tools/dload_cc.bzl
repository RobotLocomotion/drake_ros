# -*- python -*-

"""
The purpose of these rules is to support the propagation of runtime information
that is necessary for the execution of C/C++ binaries and tests that require it.
"""

load(
    "//tools:dload.bzl",
    "do_dload_shim",
    "get_dload_shim_attributes",
)

_DLOAD_CC_SHIM_TEMPLATE = """\
#include "ros2/tools/dload_shim.h"

int main(int argc, const char * argv[]) {{
  const char * executable_path = "{executable_path}";
  std::vector<const char *> names = {names};
  std::vector<std::vector<const char *>> actions = {actions};
  return do_dload_shim(argc, argv, executable_path, names, actions);
}}
"""

def _to_cc_list(collection):
    """Turn collection into a C++ aggregate initializer expression."""
    return "{" + ", ".join(collection) + "}"

def _dload_cc_shim_impl(ctx):
    return do_dload_shim(ctx, _DLOAD_CC_SHIM_TEMPLATE, _to_cc_list)

dload_cc_shim = rule(
    attrs = get_dload_shim_attributes(),
    implementation = _dload_cc_shim_impl,
    output_to_genfiles = True,
)
"""
Generates a C++ shim that can inject runtime environment information for
C/C++ binaries that have such requirements. Using a C++ shim for C++ binaries
simplifies UX during debugging sessions, as fork-follow behavior in common
debuggers like gdb and lldb makes it transparent.

This shim uses a sentinel environment variable so that it only modifies the
environment once. Any nested shims will use the top-level shim's environment.

See do_dload_shim() documentation for further reference.
"""

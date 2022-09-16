load("//tools:execute.bzl", "execute_or_fail")

VERSION_FILE_TEMPLATE = \
"""
# -*- python -*-

PYTHON_VERSION = "{}"
PYTHON_EXTENSION_SUFFIX = "{}"
"""

BUILD_FILE_TEMPLATE = \
"""
# -*- python -*-

package(default_visibility = ["//visibility:public"])

cc_library(
  name = "headers",
  hdrs = glob(
    include = ["include/**/*.*"],
    exclude_directories = 1,
  ),
  includes = {},
)

cc_library(
  name = "libs",
  linkopts = {},
  deps = [":headers"],
)
"""

def _impl(repo_ctx):
    python_interpreter = repo_ctx.which("python3")
    if python_interpreter == None:
        fail("No python3 interpreter found in PATH")
    python_config = repo_ctx.which("python3-config")
    if python_config == None:
        fail("No python3-config utility found in PATH")

    python_version = execute_or_fail(
        repo_ctx, [
            python_interpreter, "-c" ,
            "import sys; v = sys.version_info;" +
            "print('{}.{}'.format(v.major, v.minor))"
        ]
    ).stdout.strip()

    extension_suffix = execute_or_fail(
        repo_ctx, [python_config, "--extension-suffix"]
    ).stdout.strip()

    repo_ctx.file(
        "version.bzl",
        content = VERSION_FILE_TEMPLATE.format(
            python_version, extension_suffix
        ),
        executable = False
    )

    cflags = execute_or_fail(
        repo_ctx, [python_config, "--includes"]
    ).stdout.strip().split(" ")

    includes = []
    for cflag in cflags:
        if not cflag.startswith("-I"):
            continue
        include = cflag[2:]
        sandboxed_include = "include/{}".format(
            include.replace("/", "_"))
        if sandboxed_include in includes:
            continue
        repo_ctx.symlink(include, sandboxed_include)
        includes.append(sandboxed_include)

    linkopts = execute_or_fail(
        repo_ctx, [python_config, "--ldflags"]
    ).stdout.strip().split(" ")

    libpython = "python{}".format(python_version)
    links_libpython = False
    for opt in linkopts:
        if opt.startswith("-l") and libpython in opt:
            links_libpython = True
            break
    if not links_libpython:
        linkopts.append("-l{}".format(libpython))

    repo_ctx.file(
        "BUILD.bazel",
        content = BUILD_FILE_TEMPLATE.format(
            includes, linkopts
        ),
        executable = False
    )

cpython_local_repository = repository_rule(
    implementation = _impl,
    local = True,
    configure = True
)

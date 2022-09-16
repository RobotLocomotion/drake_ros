## Infrastructure to use ROS 2 from a Bazel workspace

This package encapsulates all the machinery to pull a ROS 2 workspace install
space or a subset thereof as a Bazel repository. Both system-installed binary
distributions and source builds can pulled in this way, whether symlink- or
merged-installed.

A single repository rule, `ros2_local_repository()`, is the sole entrypoint.
This rule heavily relies on two Python packages:

- The `cmake_tools` Python package, which provides an idiomatic API to collect
  a CMake project's exported configuration (see Note 1).
- The `ros2bzl` Python package, which provides tools to crawl a ROS 2 workspace
  install space, collects CMake packages' exported configuration, collect Python
  packages' egg metadata, symlink relevant directories and files, and generates
  a root BUILD.bazel file that recreates the dependency graph in the workspace.
  This package constitutes the backbone of the `generate_repository_files.py`
  Python binary which `ros2_local_repository()` invokes.

**Note 1**
: [`rules_foreign_cc`](https://github.com/bazelbuild/rules_foreign_cc) tooling
  was initially considered but later discarded, as it serves a fundamentally
  different purpose. This infrastructure does **not** build CMake projects
  within Bazel (like `rules_foreign_cc` does), it exposes the artifacts and
  build configuration of pre-installed CMake projects for Bazel packages to
  depend on.

### Repository layout

A ROS 2 local repository has the following layout:

```
  .
  ├── BUILD.bazel
  ├── rmw_isolation
  │   └── BUILD.bazel
  ├── distro.bzl
  ├── common.bzl
  ├── ros_cc.bzl
  ├── ros_py.bzl
  └── rosidl.bzl
```

Note that all files and subdirectories that are mere implementation details
have been excluded from this layout.

#### Targets

For each package in the underlying ROS 2 workspace install space, depending on
the artifacts it generates, the following targets may be found at the root
`BUILD.bazel` file:

- A `<package_name>_share` filegroup for files in the `<package_prefix>/share`
  directory but excluding those files that are build system specific.
- A `<package_name>_cc` C/C++ library for C++ libraries, typically found in
  the `<package_prefix>/lib` directory. In addition to headers, compiler flags,
  linker flags, etc., C/C++ library targets for upstream packages that are
  immediate dependencies are declared as dependencies themselves.
- A `<package_name>_py` Python library for Python eggs in the
  `<package_prefix>/lib/python*/site-packages` directory. All Python library
  targets for upstream packages that are immediate dependencies are declared
  as dependencies themselves.
- A `<package_name>_defs` filegroup for interface definition files (.msg, .srv,
  .action, and .idl files) in the `<package_prefix>/share` directory.
- A `<package_name>_c` C/C++ library for C libraries. Typically an alias of the
  `<package>_cc` target if C and C++ libraries cannot be told apart.
- A `<package_name>_transitive_py` Python library if the package does not
  install any Python libraries but it depends on (and it is a dependency of)
  packages that do. This helps maintain the dependency graph (as Python library
  targets can only depend on other Python library targets).
- A `<package_name>_<executable_name>` Python binary for each executable
  installed at the package-level (i.e. under `lib/<package_name>`, where
  `ros2 run` can find them).
- An `<executable_name>` Python binary wrapper for each executable of *any*
  kind (Python, shell script, compiled binary, etc.) installed under the
  `<package_prefix>/bin` directory (and thus accessible via `$PATH` when
  sourcing the workspace install space). These executables are exposed as
  Python binaries for simplicty.

#### Rules

To build C++ binaries and tests that depend on ROS 2, `ros_cc_binary` and
`ros_cc_test` rules are available in the `ros_cc.bzl` file. These rules,
equivalent to the native `cc_binary` and `cc_test` rules, ensure these binaries
run in an environment that is tightly coupled with the underlying ROS 2
workspace install space.

To build Python binaries and tests that depend on ROS 2, `ros_py_binary` and
`ros_pytest` rules are available in the `ros_py.bzl` file. These rules,
equivalent to the native `py_binary` and `py_test` rules, ensure these binaries
run in an environment that is tightly coupled with the underlying ROS 2
workspace install space.

To generate and build ROS 2 interfaces, a `rosidl_interfaces_group` rule is
available in the `rosidl.bzl` file. This rule generates C++ and Python code
for the given ROS 2 interface definition files and builds them using what is
available in the ROS 2 workspace install space: code generators, interface
definition translators, runtime dependencies, etc. Several targets are created,
following strict naming conventions (e.g. C++ and Python interface libraries
carry `_cc` and `_py` suffixes, respectively), though finer-grained control over
what is generated and built can be achieved through other rules available in
the same file. By default, these naming conventions allow downstream
`rosidl_interfaces_group` rules to depend on upstream `rosidl_interface_group`
rules.

#### Tools

The `rmw_isolation` subpackage provides C++ and Python `isolate_rmw_by_path`
APIs to enforce RMW network isolation. To that end, a unique path must be
provided (such as Bazel's `$TEST_TMPDIR`).

**DISCLAIMER**
: Isolation relies on `rmw`-specific configuration. Support is available for
  Tier 1 `rmw` implementations only. Collision rates are below 1% but not null.
  Use with care.

#### Metadata

The `distro.bzl` file bears relevant ROS 2 workspace metadata for rules, tools,
and downstream packages to use.

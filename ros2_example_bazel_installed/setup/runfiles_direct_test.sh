#!/bin/bash
set -eux

# This is run directly, not via Bazel, meant to test #105.
# TODO(eric.cousineau): Remove this pending #107.
bazel clean
bazel build //:runfiles_cc_test //:runfiles_py_test
./bazel-bin/runfiles_cc_test
./bazel-bin/runfiles_py_test

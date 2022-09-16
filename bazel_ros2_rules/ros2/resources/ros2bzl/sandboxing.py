"""
Utility functions to aid repository sandboxing of dependencies
external to the enclosing Bazel workspace.
"""

import os


def make_path_mapping(name, mapping):
    """
    Makes a function that maps paths outside a Bazel workspace
    to paths within a repository in that workspace.

    For paths to be found among runfiles in runtime, set the
    ``external`` keyword argument to True.

    :param name: name of the enclosing repository
    :param mapping: a key-value mapping from outer directory paths
      to inner directory paths
    :returns: callable that takes a path outside the workspace
      and yields a path inside the workspace, and optionally takes
      an ``external`` keyword argument.
    """
    def _map(path, external=False):
        path = os.path.normpath(path)
        for outer_path, inner_path in mapping.items():
            if path.startswith(outer_path):
                path = os.path.normpath(
                    path.replace(outer_path, inner_path)
                )
                if external:
                    path = os.path.join(name, path)
                return path
        return path

    return _map

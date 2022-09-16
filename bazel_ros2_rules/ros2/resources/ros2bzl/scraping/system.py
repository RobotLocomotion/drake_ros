"""
Utility functions to aid compilation and linkage configuration scraping by
resorting to (Linux) system-wide conventions and tooling.
"""

from functools import lru_cache
import os
import re
import subprocess
import sys

# Standard include files' search paths for compilers in Linux systems.
# Useful to detect system includes in package exported configuration.
DEFAULT_INCLUDE_DIRECTORIES = ['/usr/include', '/usr/local/include']


def is_system_include(include_path):
    """
    Checks whether `include_path` is in a system include directory
    i.e. known to compilers.
    """
    include_path = os.path.realpath(include_path)
    return any(
        include_path.startswith(path)
        for path in DEFAULT_INCLUDE_DIRECTORIES)


@lru_cache(maxsize=1)
def system_link_dirs():
    """Get standard search paths for the linker."""
    link_dirs = set()
    output = subprocess.run(
        ['ld', '--verbose'],
        check=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        encoding='utf8'
    ).stdout.strip()

    for directory in re.findall(r'SEARCH_DIR\("=([^=]+)"\)', output):
        link_dirs.add(directory)
    # Filter empty strings
    return tuple([d for d in link_dirs if d])


@lru_cache(maxsize=1)
def system_shared_lib_dirs():
    """Get standard search paths for the dynamic runtime loader."""
    lib_dirs = set()
    output = subprocess.run(
        ['ldconfig', '-XN', '--verbose'],
        check=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        encoding='utf8'
    ).stdout.strip()

    for directory in re.findall(r'([^:\t\n]+):', output):
        lib_dirs.add(directory)
    # Workaround Singularity redirects, e.g. for `--nv`.
    lib_dirs.add("/.singularity.d/libs")
    # Filter empty strings
    return tuple([d for d in lib_dirs if d])


def is_system_library(library_path):
    """
    Checks whether `library_path` is in a system library directory
    i.e. known to the linker or dynamic loader
    """
    library_path = os.path.realpath(library_path)
    return any(
        library_path.startswith(path)
        for path in system_link_dirs() + system_shared_lib_dirs())


def find_library_path(library_name, link_directories=None, link_flags=None):
    """
    Finds the path to a library.

    To do so it relies on the GNU `ld` linker and its library naming spec,
    effectively reusing all of its search logic.

    :param library_name: name of the library to be searched
    :param link_directories: optional list of directories to search in
    :param link_flags: optional list of linker flags to account for
    :returns: the path to the library if found, None otherwise
    """
    # Adapted from ctypes.util.find_library_path() implementation.
    pattern = r'/(?:[^\(\)\s]*/)*lib{}\.[^\(\)\s]*'.format(library_name)

    paths = []
    if link_directories:
        paths.extend(link_directories)
    paths.extend(set(os.environ.get('LIBRARY_PATH', '').split(':')))
    paths.extend(set(os.environ.get('LD_LIBRARY_PATH', '').split(':')))
    paths.extend(system_link_dirs())
    paths.extend(system_shared_lib_dirs())

    cmd = ['ld', '-t']
    if link_flags:
        cmd.extend(link_flags)
    for path in paths:
        cmd.extend(['-L', path])
    cmd.extend(['-o', os.devnull, '-l' + library_name])
    try:
        out = subprocess.run(
            cmd,
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            encoding='utf8'
        ).stdout.strip()
        match = re.search(pattern, out)
        if match:
            path = match.group(0)
            # Remove double forward slashes, if any
            return os.path.join('/', os.path.relpath(path, '/'))
    except Exception:
        pass
    return None


LDD_LINE_PATTERN = re.compile(r' => (/(?:[^\(\)\s]*/)*lib[^\(\)\s]*)')


def find_library_dependencies(library_path):
    """
    Lists all shared library dependencies of a given library.

    To do so, it relies on the `ldd` tool as found in Linux.

    :param library_path: path to the library to be inspected
    :returns: a generator that iterates over the paths to
      library dependencies
    """
    try:
        lines = subprocess.run(
            ['ldd', library_path],
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            encoding='utf8'
        ).stdout.strip().split('\n')
        for line in lines:
            match = LDD_LINE_PATTERN.search(line.strip())
            if match:
                yield match.group(1)
    except Exception:
        pass
    return

"""
Provides a Python API to CMake's server mode.
"""

import os
import shutil
import subprocess

from .packages import get_packages_with_prefixes
from .server_mode import server_mode


def configure_file(src, dest, subs):
    with open(src, 'r') as f:
        text = f.read()
    for old, new in subs.items():
        text = text.replace(old, new)
    with open(dest, 'w') as f:
        f.write(text)


def build_then_install(project_path, *args, build_path='build'):
    if not os.path.isabs(build_path):
        build_path = os.path.join(project_path, build_path)
    if os.path.exists(build_path):
        shutil.rmtree(build_path)
    os.makedirs(build_path)
    subprocess.run([
        'cmake', '-G', 'Unix Makefiles', *args, project_path
    ], cwd=build_path, check=True)
    subprocess.run(['make', 'install'], cwd=build_path, check=True)

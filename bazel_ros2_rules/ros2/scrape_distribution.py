#!/usr/bin/env python3

"""
Scrapes ROS 2 workspaces and exposes their artifacts through a Bazel local repository.

This script generates:

- a BUILD.bazel file, with targets for all C/C++ libraries, Python libraries, executables, and share
  data files found in each scrapped ROS 2 package
- a distro.bzl file, with ROS 2 metadata as constants
"""

import argparse
import json
import os
import sys

from ros2bzl.scraping import scrape_distribution


def parse_arguments():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument(
        '-o', '--output', type=argparse.FileType('w'), default=sys.stdout,
        help='Output file to write distro information to, in JSON format'
    )
    parser.add_argument(
        '-i', '--include-package', action='append',
        dest='include_packages', metavar='PACKAGE_NAME', default=[],
        help='Packages to be included (plus their recursive dependencies)'
    )
    parser.add_argument(
        '-e', '--exclude-package', action='append',
        dest='exclude_packages', metavar='PACKAGE_NAME', default=[],
        help='Packages to be explicitly excluded'
    )
    return parser.parse_args()


def main():
    args = parse_arguments()

    distro = scrape_distribution(
        set(args.include_packages),
        set(args.exclude_packages))

    json.dump(distro, args.output, default=list)


if __name__ == '__main__':
    main()

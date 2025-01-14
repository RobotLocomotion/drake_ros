import argparse
import sys
from subprocess import Popen

# This script spawns a bunch of talker-listener pairs
# publishing on the same topic. They are isolated using
# the network isolation mechanism, and hence should not
# cross talk.

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--number_of_isolated_pairs", type=int, default=5)
    args = parser.parse_args()

    subprocess_list = []
    for i in range(args.number_of_isolated_pairs):
        subprocess_list.append(Popen(['external/bazel_ros2_rules/network_isolation/isolate',
                                       sys.executable, 'test/talker_listener.py',
                                      '--id', str(i)]))

    for process in subprocess_list:
        process.wait()

if __name__ == "__main__":
    main()

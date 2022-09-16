import os
import sys

from bazel_tools.tools.python.runfiles import runfiles

SHIMMED_SENTINEL = "_BAZEL_ROS2_RULES_SHIMMED";

def do_dload_shim(executable_path, names, actions):
    """
    Call exec() on another executable with arguments and modified environment
    variables.
    This is meant to be called by a generated shim executable.

    :param argc: count of arguments in argv.
    :param argv: process arguments.
    :param executable_path: path to an executable to execute.
    :param names: environment variables to be modified.
    :param actions: actions to be performed on each named environment variable.
    """
    argv = sys.argv
    r = runfiles.Create()
    # NOTE(hidmic): unlike its C++ equivalent, Python runfiles'
    # builtin tools will only look for runfiles in the manifest
    # if there is a manifest
    runfiles_dir = r.EnvVars()['RUNFILES_DIR']

    def rlocation(path):
        return r.Rlocation(path) or os.path.join(runfiles_dir, path)

    if SHIMMED_SENTINEL not in os.environ:
        for name, action in zip(names, actions):  # noqa
            action_type, action_args = action[0], action[1:]
            if action_type == 'replace':
                assert len(action_args) == 1
                value = action_args[0]
            elif action_type == 'set-if-not-set':
                assert len(action_args) == 1
                if name in os.environ:
                    continue
                value = action_args[0]
            elif action_type == 'path-replace':
                assert len(action_args) == 1
                value = rlocation(action_args[0])
            elif action_type == 'path-prepend':
                assert len(action_args) > 0
                value = ':'.join([rlocation(path) for path in action_args])
                if name in os.environ:
                    value += ':' + os.environ[name]
            else:
                assert False  # should never get here
            if '$PWD' in value:
                value = value.replace('$PWD', os.getcwd())
            os.environ[name] = value
        os.environ[SHIMMED_SENTINEL] = ""

    real_executable_path = r.Rlocation(executable_path)  # noqa
    argv = [real_executable_path] + argv[1:]
    os.execv(real_executable_path, argv)

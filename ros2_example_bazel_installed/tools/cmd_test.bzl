# -*- python -*-

def cmd_test(name, cmd, **kwargs):
    native.sh_test(
        name = name,
        srcs = ["//tools:cmd_exec.sh"],
        args = cmd,
        **kwargs
    )

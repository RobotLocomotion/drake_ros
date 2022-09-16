# -*- python -*-

def _environment_repository_impl(repo_ctx):
    environ = {
        envvar: repo_ctx.os.environ.get(envvar, "")
        for envvar in repo_ctx.attr._envvars}
    repo_ctx.file(
        "BUILD.bazel",
        content = "# Empty build file to mark repository root.\n",
        executable = False,
    )
    repo_ctx.file(
        "environ.bzl",
        content = "\n".join([
            "{}=\"{}\"".format(name, value)
            for name, value in environ.items()
        ]),
        executable = False
    )

def environment_repository(name, envvars):
    rule = repository_rule(
        attrs = {
            "_envvars": attr.string_list(default = envvars)
        },
        implementation = _environment_repository_impl,
        environ = envvars,
        local = True,
    )
    rule(name = name)

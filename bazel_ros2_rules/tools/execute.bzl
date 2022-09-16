# -*- python -*-

def execute_or_fail(repo_ctx, cmd, **kwargs):
    exec_result = repo_ctx.execute(cmd, **kwargs)
    if exec_result.return_code != 0:
        error_message ="'{}' exited with {}".format(
            " ".join([str(token) for token in cmd]),
            exec_result.return_code
        )
        if exec_result.stdout:
            error_message += "\n--- captured stdout ---\n"
            error_message += exec_result.stdout
        if exec_result.stderr:
            error_message += "\n--- captured stderr ---\n"
            error_message += exec_result.stderr
        fail("Failed to setup @{} repository: {}".format(
            repo_ctx.name, error_message
        ))
    return exec_result

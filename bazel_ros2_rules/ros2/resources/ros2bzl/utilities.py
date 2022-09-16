import functools
import json


class StarlarkEncoder(json.JSONEncoder):
    # Use JSON format as a basis
    def default(self, obj):
        if isinstance(obj, bool):
            return repr(obj)
        return super().default(obj)


def to_starlark_string_dict(d):
    encoder = StarlarkEncoder()
    return {
        k: encoder.encode(v)
        for k, v in d.items()
    }


def interpolate(template, config):
    content = template
    for key, value in config.items():
        content = content.replace('@{}@'.format(key), value)
    return content

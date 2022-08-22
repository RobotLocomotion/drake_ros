# -*- python -*-

AmentIndex = provider(
    fields = ["prefix"],
)

AmentIndexes = provider(
    fields = ["prefixes"],
)

def _ament_index_prefixes_aspect_impl(target, ctx):
    prefixes = []
    if AmentIndex in target:
        prefixes.append(target[AmentIndex].prefix)
    if hasattr(ctx.rule.attr, "data"):
        for data in ctx.rule.attr.data:
            if AmentIndexes in data:
                prefixes.extend(data[AmentIndexes].prefixes)
    if hasattr(ctx.rule.attr, "deps"):
        for dep in ctx.rule.attr.deps:
            if AmentIndexes in dep:
                prefixes.extend(dep[AmentIndexes].prefixes)
    return [AmentIndexes(prefixes = prefixes)]

ament_index_prefixes = aspect(
  implementation = _ament_index_prefixes_aspect_impl,
  attr_aspects = ["deps", "data"],
)

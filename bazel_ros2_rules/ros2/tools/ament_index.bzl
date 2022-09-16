# -*- python -*-

AmentIndex = provider(
    fields = ["prefix"],
)
"""
Provide the location of an ament resource index.

This provider is intended to be returned by a rule.

See rosidl_generate_ament_index_entry()
"""

AggregatedAmentIndexes = provider(
    fields = ["prefixes"],
)
"""
Provides the location of multiple ament resource indexes.

This is intended to be aggregated by an aspect, and should not be used by
rules because it prevents an aspect from aggregating the information again.

See ament_index_prefixes for more info.
"""

def _ament_index_prefixes_aspect_impl(target, ctx):
    prefixes = []
    if AmentIndex in target:
        prefixes.append(target[AmentIndex].prefix)
    if hasattr(ctx.rule.attr, "data"):
        for data in ctx.rule.attr.data:
            if AggregatedAmentIndexes in data:
                prefixes.extend(data[AggregatedAmentIndexes].prefixes)
    if hasattr(ctx.rule.attr, "deps"):
        for dep in ctx.rule.attr.deps:
            if AggregatedAmentIndexes in dep:
                prefixes.extend(dep[AggregatedAmentIndexes].prefixes)
    return [AggregatedAmentIndexes(prefixes = prefixes)]

ament_index_prefixes = aspect(
    implementation = _ament_index_prefixes_aspect_impl,
    attr_aspects = ["deps", "data"],
)
"""
Recursively aggregates AmentIndex prefixes from dependencies into one provider.
"""

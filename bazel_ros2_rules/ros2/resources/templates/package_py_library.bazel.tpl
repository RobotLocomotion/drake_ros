py_library(
    name = @name@,
    srcs = glob(["{}/**/*.py".format(x) for x in @tops@]),
    data = glob(
       include=[
         "{}/**/*.*".format(x) for x in @tops@
       ] + [
         "{}/*".format(x) for x in @eggs@
       ],
       exclude=["**/*.py", "**/*.so"],
    ) + @data@,
    imports = @imports@,
    deps = @deps@,
)

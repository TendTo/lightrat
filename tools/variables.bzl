LIGHTRAT_NAME = "lightrat"
LIGHTRAT_VERSION = "0.0.1"
LIGHTRAT_AUTHOR = "Ernesto Casablanca"
LIGHTRAT_AUTHOR_EMAIL = "casablancaernesto@gmail.com"
LIGHTRAT_DESCRIPTION = "Lightweight library for rational numbers"
LIGHTRAT_HOMEPAGE = "https://github.com/TendTo/lightrat"
LIGHTRAT_SOURCE = "https://github.com/TendTo/lightrat"
LIGHTRAT_TRACKER = "https://github.com/TendTo/lightrat/issues"
LIGHTRAT_LICENSE = "Apache-2.0"

def _expose_variable_impl(_):
    pass

expose_variable = rule(
    implementation = _expose_variable_impl,
    attrs = {
        "value": attr.string(
            mandatory = True,
            doc = "The value of the variable to expose.",
        ),
    },
)

def _make_var_substitution_impl(ctx):
    vars = dict(ctx.attr.variables)

    # Python
    py_runtime = ctx.toolchains[ctx.attr._python_toolchain].py3_runtime
    major = py_runtime.interpreter_version_info.major
    minor = py_runtime.interpreter_version_info.minor
    implementation = py_runtime.implementation_name
    if implementation == "cpython":
        tag = "cp" + str(major) + str(minor)
        vars["PYTHON_ABI_TAG"] = tag
        vars["PYTHON_TAG"] = tag
    else:
        fail("This rule only supports CPython.")

    # lightrat
    vars["LIGHTRAT_NAME"] = LIGHTRAT_NAME
    vars["LIGHTRAT_VERSION"] = LIGHTRAT_VERSION
    vars["LIGHTRAT_VERSION_MAJOR"], vars["LIGHTRAT_VERSION_MINOR"], vars["LIGHTRAT_VERSION_PATCH"] = LIGHTRAT_VERSION.split(".")
    vars["LIGHTRAT_AUTHOR"] = LIGHTRAT_AUTHOR
    vars["LIGHTRAT_AUTHOR_EMAIL"] = LIGHTRAT_AUTHOR_EMAIL
    vars["LIGHTRAT_DESCRIPTION"] = LIGHTRAT_DESCRIPTION
    vars["LIGHTRAT_HOMEPAGE"] = LIGHTRAT_HOMEPAGE
    vars["LIGHTRAT_LICENSE"] = LIGHTRAT_LICENSE
    vars["LIGHTRAT_SOURCE"] = LIGHTRAT_SOURCE
    vars["LIGHTRAT_TRACKER"] = LIGHTRAT_TRACKER

    return [platform_common.TemplateVariableInfo(vars)]

make_var_substitution = rule(
    implementation = _make_var_substitution_impl,
    attrs = {
        "variables": attr.string_dict(),
        "_python_toolchain": attr.string(default = "@rules_python//python:toolchain_type"),
    },
    doc = """Provides a set of variables to the template engine.
Variables are passed as a dictionary of strings.
The keys are the variable names, and the values are the variable values.

It also comes with a set of default variables that are always available:
- LIGHTRAT_NAME: The name of the lightrat library.
- LIGHTRAT_VERSION: The version of the lightrat library.
- LIGHTRAT_AUTHOR: The author of the lightrat library.
- LIGHTRAT_AUTHOR_EMAIL: The author email of the lightrat library.
- LIGHTRAT_DESCRIPTION: The description of the lightrat library.
- LIGHTRAT_HOMEPAGE: The homepage of the lightrat library.
- LIGHTRAT_LICENSE: The license of the lightrat library.
- PYTHON_ABI_TAG: The Python ABI tag (e.g., cp36, cp311).
- PYTHON_TAG: The Python tag (e.g., cp36, cp311).

Example:
```python
make_var_substitution(
    variables = {
        "MY_VARIABLE": "my_value",
    },
)
```

This will make the variable `MY_VARIABLE` available to the template engine.
""",
    toolchains = [
        "@rules_python//python:toolchain_type",
    ],
)

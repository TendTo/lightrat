"""Provide a centralised place to define rules for building C++ code.
This makes it easy to change the build configuration for all C++ rules in the project at once.
Inspired by Drake's drake.bzl file https://github.com/RobotLocomotion/drake/blob/master/tools/drake.bzl.
"""

load("@nanobind_bazel//:build_defs.bzl", "nanobind_extension")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_shared_library", "cc_test")
load("@rules_cc//cc:cc_static_library.bzl", "cc_static_library")

# The CXX_FLAGS will be enabled for all C++ rules in the project
# building with any linux compiler.
CXX_FLAGS = [
    "-std=c++20",
    "-Wall",
    "-Wattributes",
    "-Wdeprecated",
    "-Wdeprecated-declarations",
    "-Wextra",
    "-Wignored-qualifiers",
    "-Wold-style-cast",
    "-Woverloaded-virtual",
    "-Wpedantic",
    "-Wshadow",
    "-Werror",
    #    "-Wno-unknown-pragmas",  # Ignore unknown pragmas
]

# The CLANG_FLAGS will be enabled for all C++ rules in the project when
# building with clang.
CLANG_FLAGS = CXX_FLAGS + [
    "-Wabsolute-value",
    "-Winconsistent-missing-override",
    "-Wnon-virtual-dtor",
    "-Wreturn-stack-address",
    "-Wsign-compare",
]

# The GCC_FLAGS will be enabled for all C++ rules in the project when
# building with gcc.
GCC_FLAGS = CXX_FLAGS + [
    "-Wlogical-op",
    "-Wnon-virtual-dtor",
    "-Wreturn-local-addr",
    "-Wunused-but-set-parameter",
]

# The MSVC_CL_FLAGS will be enabled for all C++ rules in the project when
# building with MSVC.
MSVC_CL_FLAGS = [
    "-W4",
    "-WX",
    #    "-wd4068",  # Ignore unknown pragmas
]

# The CLANG_CL_FLAGS will be enabled for all C++ rules in the project when
# building with clang-cl.
CLANG_CL_FLAGS = [
    "-Wabsolute-value",
    "-Winconsistent-missing-override",
    "-Wnon-virtual-dtor",
    "-Wreturn-stack-address",
    "-Wsign-compare",
]

# The test configuration flags for each compiler.
# Mostly used to remove the -Werror flag, since in same cases we want to test
# borderline cases that would often be caught by a compiler warning.
GCC_TEST_FLAGS = [flag for flag in GCC_FLAGS if flag != "-Werror"]
CLANG_TEST_FLAGS = [flag for flag in CLANG_FLAGS if flag != "-Werror"]
MSVC_CL_TEST_FLAGS = [flag for flag in MSVC_CL_FLAGS if flag != "-WX"]
CLANG_CL_TEST_FLAGS = [flag for flag in CLANG_CL_FLAGS if flag != "-WX"]

# Default defines for all C++ rules in the project.
LIGHTRAT_DEFINES = []

def _get_copts(rule_copts, cc_test = False):
    """Alter the provided rule specific copts, adding the platform-specific ones.

    When cc_test is True, the corresponding test flags will be added.
    It should only be set on cc_test rules or rules that are boil down to cc_test rules.

    Args:
        rule_copts: The copts passed to the rule.
        cc_test: Whether the rule is a cc_test rule.

    Returns:
        A list of copts.
    """
    return select({
        "//tools:gcc_build": (GCC_TEST_FLAGS if cc_test else GCC_FLAGS) + rule_copts,
        "//tools:clang_build": (CLANG_CL_TEST_FLAGS if cc_test else CLANG_FLAGS) + rule_copts,
        "//tools:msvc_cl_build": (MSVC_CL_TEST_FLAGS if cc_test else MSVC_CL_FLAGS) + rule_copts,
        "//tools:clang_cl_build": (CLANG_CL_TEST_FLAGS if cc_test else CLANG_CL_FLAGS) + rule_copts,
        "//conditions:default": CXX_FLAGS + rule_copts,
    })

def _get_defines(rule_defines):
    """Alter the provided rule specific defines, adding the platform-specific ones.

    Args:
        rule_defines: The defines passed to the rule.

    Returns:
        A list of defines.
    """
    return rule_defines + LIGHTRAT_DEFINES + select({
        "//tools:debug_build": [],
        "//conditions:default": [],
    }) + select({
        "//tools:python_build": ["LIGHTRAT_PYTHON"],
        "//conditions:default": [],
    }) + select({
        "//tools:explicit_constructor_build": ["LIGHTRAT_EXPLICIT_CONSTRUCTOR=explicit"],
        "//conditions:default": ["LIGHTRAT_EXPLICIT_CONSTRUCTOR="],
    }) + select({
        "//tools:explicit_cast_build": ["LIGHTRAT_EXPLICIT_CAST=explicit"],
        "//conditions:default": ["LIGHTRAT_EXPLICIT_CAST="],
    }) + select({
        "//tools:infinity_build": ["LIGHTRAT_INFINITY"],
        "//conditions:default": [],
    }) + select({
        "//tools:nan_build": ["LIGHTRAT_NAN"],
        "//conditions:default": [],
    }) + select({
        "//tools:thread_safe_build": ["LIGHTRAT_THREAD_SAFE"],
        "//conditions:default": [],
    })

def _get_static(rule_linkstatic):
    """Alter the provided linkstatic, by considering the platform-specific one.

    The files are linked statically by default.

    Args:
        rule_linkstatic: The linkstatic passed to the rule.

    Returns:
        The linkstatic value to use
    """
    if rule_linkstatic != None:
        return rule_linkstatic
    return select({
        "//tools:static_build": True,
        "//tools:dynamic_build": False,
        "//conditions:default": True,
    })

def _get_features(rule_features):
    """Alter the provided features, adding the platform-specific ones.

    Args:
        rule_features: The features passed to the rule.

    Returns:
        A list of features.
    """
    return rule_features + select({
        "//tools:dynamic_build": ["external_include_paths"],
        "//tools:static_build": ["external_include_paths", "fully_static_link"],
        "//conditions:default": ["external_include_paths"],
    })

def lightrat_cc_library(
        name,
        hdrs = None,
        srcs = None,
        deps = None,
        copts = [],
        linkstatic = None,
        defines = [],
        implementation_deps = [],
        features = [],
        **kwargs):
    """Creates a rule to declare a C++ library.

    Args:
        name: The name of the library.
        hdrs: A list of header files to add. Will be inherited by dependents.
        srcs: A list of source files to compile.
        deps: A list of dependencies. Will be inherited by dependents.
        implementation_deps: A list of dependencies that are only needed for this target.
        copts: A list of compiler options.
        linkstatic: Whether to link statically.
        defines: A list of compiler defines used when compiling this target and its dependents.
        features: A list of features to use when compiling this target and its dependents.
        **kwargs: Additional arguments to pass to cc_library.
    """
    cc_library(
        name = name,
        hdrs = hdrs,
        srcs = srcs,
        deps = deps,
        implementation_deps = implementation_deps,
        copts = _get_copts(copts),
        linkstatic = _get_static(linkstatic),
        defines = _get_defines(defines),
        features = _get_features(features),
        **kwargs
    )

def lightrat_cc_shared_library(name, deps, shared_lib_name = None, **kwargs):
    """Creates a rule to declare a C++ shared library.

    Args:
        name: The name of the library.
        deps: A list of dependencies. Will be inherited by dependents.
        shared_lib_name: The name of the shared library file.
            If the extension is not provided, it will be inferred based on the platform.
            If not provided, it will be inferred from the name.
        **kwargs: Additional arguments to pass to cc_shared_library.
    """
    if shared_lib_name != None and "." not in shared_lib_name:
        shared_lib_name = select({
            "//tools:msvc_cl_build": shared_lib_name + ".dll",
            "//tools:clang_cl_build": shared_lib_name + ".dll",
            "//conditions:default": "lib" + shared_lib_name + ".so",
        })

    cc_shared_library(
        name = name,
        deps = deps,
        shared_lib_name = shared_lib_name,
        **kwargs
    )

def lightrat_cc_static_library(name, deps, **kwargs):
    """Creates a rule to declare a C++ static library.

    Args:
        name: The name of the library.
        deps: A list of dependencies. Will be inherited by dependents.
        **kwargs: Additional arguments to pass to cc_library.
    """
    cc_static_library(
        name = name,
        deps = deps,
        **kwargs
    )

def lightrat_cc_binary(
        name,
        srcs = None,
        deps = None,
        copts = [],
        linkstatic = None,
        defines = [],
        features = [],
        **kwargs):
    """Creates a rule to declare a C++ binary.

    Args:
        name: The name of the binary.
        srcs: A list of source files to compile.
        deps: A list of dependencies.
        copts: A list of compiler options.
        linkstatic: Whether to link statically.
        defines: A list of compiler defines used when compiling this target.
        features: A list of features to add to the binary.
        **kwargs: Additional arguments to pass to cc_binary.
    """
    cc_binary(
        name = name,
        srcs = srcs,
        deps = deps,
        copts = _get_copts(copts),
        linkstatic = _get_static(linkstatic),
        defines = _get_defines(defines),
        features = _get_features(features),
        **kwargs
    )

def lightrat_cc_test(
        name,
        srcs = None,
        copts = [],
        tags = [],
        defines = [],
        features = [],
        **kwargs):
    """Creates a rule to declare a C++ unit test.

    Note that for almost all cases, lightrat_cc_googletest should be used instead of this rule.

    By default, sets size="small" because that indicates a unit test.
    If a list of srcs is not provided, it will be inferred from the name by appending .cpp.
    For example, lightrat_cc_test(name = "test_foo_bar") will look for test_foo_bar.cpp.
    Furthermore, a tag will be added for the test, based on the name, by converting the name to lowercase and removing the "test_" prefix.

    Args:
        name: The name of the test.
        srcs: A list of source files to compile.
        copts: A list of compiler options.
        tags: A list of tags to add to the test. Allows for test filtering.
        defines: A list of compiler defines used when compiling this target.
        features: A list of features to use when compiling this target and its dependents.
        **kwargs: Additional arguments to pass to cc_test.
    """
    if srcs == None:
        srcs = [name + ".cpp"]
    cc_test(
        name = name,
        srcs = srcs,
        copts = _get_copts(copts, cc_test = True),
        linkstatic = True,
        tags = tags + ["lightrat", "".join([word.lower() for word in name.split("_")][1:])],
        defines = _get_defines(defines),
        features = _get_features(features),
        **kwargs
    )

def lightrat_cc_googletest(
        name,
        srcs = None,
        deps = None,
        size = "small",
        tags = [],
        use_default_main = True,
        **kwargs):
    """Creates a rule to declare a C++ unit test using googletest.

    Always adds a deps= entry for googletest main
    (@googletest//:gtest_main).

    By default, it uses size="small" because that indicates a unit test.
    By default, it uses use_default_main=True to use GTest's main, via @googletest//:gtest_main.
    If use_default_main is False, it will depend on @googletest//:gtest instead.
    If a list of srcs is not provided, it will be inferred from the name by appending .cpp.
    For example, lightrat_cc_test(name = "test_foo_bar") will look for test_foo_bar.cpp.
    Furthermore, a tag will be added for the test, based on the name, by converting the name to lowercase and removing the "test_" prefix.

    Args:
        name: The name of the test.
        srcs: A list of source files to compile.
        deps: A list of dependencies.
        size: The size of the test.
        tags: A list of tags to add to the test. Allows for test filtering.
        use_default_main: Whether to use googletest's main.
        **kwargs: Additional arguments to pass to lightrat_cc_test.
    """
    if deps == None:
        deps = []
    if type(deps) == "select":
        if use_default_main:
            deps += select({"//conditions:default": ["@googletest//:gtest_main"]})
        else:
            deps += select({"//conditions:default": ["@googletest//:gtest"]})
    elif use_default_main:
        deps.append("@googletest//:gtest_main")
    else:
        deps.append("@googletest//:gtest")
    lightrat_cc_test(
        name = name,
        srcs = srcs,
        deps = deps,
        size = size,
        tags = tags + ["googletest"],
        **kwargs
    )

def lightrat_cc_benchmark(
        name,
        srcs = None,
        data = [],
        deps = None,
        copts = [],
        tags = [],
        defines = [],
        features = [],
        **kwargs):
    """Creates a rule to declare a C++ benchmark.

    Note that for almost all cases, lightrat_cc_googlebenchmark should be used instead of this rule.

    If a list of srcs is not provided, it will be inferred from the name by appending .cpp.
    For example, lightrat_cc_benchmark(name = "bench_foo_bar") will look for bench_foo_bar.cpp.
    Furthermore, a tag will be added for the rule, based on the name, by converting the name to lowercase and removing the "bench_" prefix.

    Args:
        name: The name of the test.
        srcs: A list of source files to compile.
        data: A list of data files to include in the test. Can be used to provide input files.
        deps: A list of dependencies.
        copts: A list of compiler options.
        tags: A list of tags to add to the test. Allows for test filtering.
        defines: A list of compiler defines used when compiling this target.
        features: A list of features to use when compiling this target and its dependents.
        **kwargs: Additional arguments to pass to cc_test.
    """
    if srcs == None:
        srcs = [name + ".cpp"]
    if deps == None:
        deps = []
    if data:
        deps.append("@rules_cc//cc/runfiles")
    cc_binary(
        name = name,
        srcs = srcs,
        data = data,
        deps = deps,
        copts = _get_copts(copts, cc_test = True),
        linkstatic = True,
        tags = tags + ["lightrat", "".join([word.lower() for word in name.split("_")][1:])],
        defines = _get_defines(defines),
        features = _get_features(features),
        **kwargs
    )

def lightrat_cc_googlebenchmark(
        name,
        srcs = None,
        deps = None,
        tags = [],
        use_default_main = True,
        **kwargs):
    """Creates a rule to declare a C++ unit test using google benchmark.

    Always adds a deps= entry to the google benchmark main (@google_benchmark//:benchmark_main).

    By default, it uses use_default_main=True to use GTest's main, via @google_benchmark//:benchmark_main.
    If use_default_main is False, it will depend on @google_benchmark//:benchmark instead.
    If a list of srcs is not provided, it will be inferred from the name by appending .cpp.
    For example, lightrat_cc_googlebenchmark(name = "bench_foo_bar") will look for bench_foo_bar.cpp.
    Furthermore, a tag will be added for the test, based on the name, by converting the name to lowercase and removing the "bench_" prefix.

    Args:
        name: The name of the test.
        srcs: A list of source files to compile.
        deps: A list of dependencies.
        tags: A list of tags to add to the test. Allows for test filtering.
        use_default_main: Whether to use googletest's main.
        **kwargs: Additional arguments to pass to lightrat_cc_test.
    """
    if deps == None:
        deps = []
    if type(deps) == "select":
        if use_default_main:
            deps += select({"//conditions:default": ["@google_benchmark//:benchmark_main"]})
        else:
            deps += select({"//conditions:default": ["@google_benchmark//:benchmark"]})
    elif use_default_main:
        deps.append("@google_benchmark//:benchmark_main")
    else:
        deps.append("@google_benchmark//:benchmark")
    lightrat_cc_benchmark(
        name = name,
        srcs = srcs,
        deps = deps,
        tags = tags + ["google_benchmark"],
        **kwargs
    )

def lightrat_cc_py_extension(name, srcs, copts = [], linkstatic = True, defines = [], features = [], **kwargs):
    """Creates a rule to declare a pybind11 extension.

    Args:
        name: The name of the extension.
        srcs: A list of source files to compile.
        copts: A list of compiler options.
        linkstatic: Whether to link statically.
        defines: A list of compiler defines used when compiling this target.
        features: A list of features to use when compiling this target and its dependents.
        **kwargs: Additional arguments to pass to pybind_extension.
    """
    nanobind_extension(
        name = name,
        srcs = srcs,
        copts = _get_copts(copts),
        linkstatic = _get_static(linkstatic),
        defines = _get_defines(defines),
        features = _get_features(features),
        **kwargs
    )

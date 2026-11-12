# Examples

Some examples of how to use the lightrat library.

## With bazel

Integrating the ligthrat library in a bazel project is pretty straightforward.
First, declare the dependency on lightrat in the `MODULE.bazel` file.
By default, bazel will look to the [Bazel central registry (BCR)](https://registry.bazel.build/home) index.
If the version you want is not there, consider overriding the dependency resolution with either this git repository or a local path.

```bzl
# MODULE.bazel

# From the BCR
bazel_dep(name = "lightrat", version = "...")

# To override with a git ref, add the following
git_override(
    module_name = "lightrat",
    commit = "...", # Commit hash you want to use
    remote = "https://github.com/TendTo/lightrat.git",
)

# To override with a local path, add the following
local_path_override(
    module_name = "lightrat",
    path = "..",
)
```

Then, add the lightrat dependency to your rules.

### CPP

```bzl
# BUILD.bazel
load("@rules_cc//cc:defs.bzl", "cc_binary")

cc_binary(
    name = "example_cpp",
    srcs = ["example.cpp"],
    deps = ["@lightrat//lightrat"],
)
```

```
bazel run //:example_cpp
```

### Python

```bzl
# BUILD.bazel
load("@rules_python//python:defs.bzl", "py_binary")

py_binary(
    name = "example_py",
    srcs = ["example.py"],
    main = "example.py",
    deps = ["@lightrat//bindings/python/lightrat"],
)
```

```
bazel run --@lightrat//tools:enable_python_build //:example_py
```

## Without bazel

To use lightrat without bazel, you will need to build and install the library first.

### CPP

First, install [gmp](https://gmplib.org/).
Then, build lightrat.
The library will be located in the build folder.

```bash
bazel run @lightrat//lightrat:install -- --destdir=$(pwd)/build
```

You can either link lightrat dynamically,

```bash
# Linux
# g++ can be replaced with clang++
g++ -std=c++20 -Ibuild/include example.cpp -Lbuild/lib -llightrat -o example
LD_LIBRARY_PATH=build/lib ./example # `examples` needs to find build/lib/liblightrat.so
```

or statically

```bash
# Linux
# g++ can be replaced with clang++
g++ -std=c++20 -Ibuild/include example.cpp -Lbuild/lib -llightrat -o example -static
./example # `example` is NOT a dynamic executable
```

### Python

Install lightrat with pip.
We provide a pre-built version on [Pypi](https://pypi.org/project/lightrat/), but you can also build it from source.

```bash
# Install the pre-built version from pypi
pip install lightrat
# Build the python library from source
pip install ..
```

You can check that the installation was successful by invoking the command

```bash
# Linux / macOS
python3 -m lightrat
# Windows
py -m lightrat
```

Finally, run the example script.

```bash
# Linux / macOS
python3 example.py
# Windows
py example.py
```

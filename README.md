# <img alt="Icon" src="docs/_static/logo.svg" align="left" width="35" height="35"> Lightrat

[![lightrat CI](https://github.com/TendTo/lightrat/actions/workflows/lightrat.yml/badge.svg)](https://github.com/TendTo/lightrat/actions/workflows/lightrat.yml)
[![Docs CI](https://github.com/TendTo/lightrat/actions/workflows/docs.yml/badge.svg)](https://github.com/TendTo/lightrat/actions/workflows/docs.yml)

Lightweight library for rational numbers.

## Installation

### From source

To use lightrat compiling it from source you will need

- [Bazel](https://bazel.build/) 8.1.1
  - We suggest using [bazelisk](https://github.com/bazelbuild/bazelisk) to manage Bazel's version.
- C++ compiler with C++20 support.  
  We tested in particular:
  - **On Linux**: [gcc](https://gcc.gnu.org/) 11.4.0
  - **On Windows**: [msvc](https://visualstudio.microsoft.com/) 14.51.36247
  - **On macOS**: [Clang/LLVM](https://clang.llvm.org/) 21.0.0

#### Using lightrat within Bazel

Add `lightrat` to your `MODULE.bazel` and depend on it directly:

```bzl
# MODULE.bazel file

bazel_dep(name = "lightrat", version = "0.0.1")
```

If you don't want to depend on the [Bazel package registry](https://registry.bazel.build/home) or need a not-yet-published version of this module, you can use a `git_override` by adding the following lines below bazel_dep in your MODULE.bazel file:

```bzl
# MODULE.bazel file

bazel_dep(name = "lightrat", version = "0.0.1")
git_override(
    module_name = "lightrat",
    commit = "aacc1c856c350a89a0fa9c43b9318a248d5f1781", # Commit hash you want to use
    remote = "https://github.com/TendTo/lightrat.git",
)
```

Then add `@lightrat` to the `deps` of your target.

#### Using lightrat outside Bazel

Clone the project and move to the root directory.

```bash
git clone https://github.com/you/lightrat.git
cd lightrat

# Linux / macOS
bazel run //lightrat:install -- --destdir=$(pwd)/build

# Windows (PowerShell or cmd)
bazel run //lightrat:install -- --destdir=%cd%\build
```

Then add `<destdir>/include` to your include path and link against the library in `<destdir>/lib`.
For example, given the following example code in `example.cpp`:

```cpp
// example.cpp

#include <iostream>
#include <lightrat/Rational.h>

int main() {
    std::cout << "r: " << lightrat::Rational<>(1, 2) << std::endl;
    return 0;
}
```

compile it with:

```bash
# The same command works when replacing g++ with clang++
g++ -std=c++20 -Ibuild/include example.cpp -Lbuild/lib -llightrat -o example
```

and run it with:

```bash
# Linux / macOS
LD_LIBRARY_PATH=build/lib ./example
```

> [!NOTE]  
> You may need administrator privileges to install the library in a system-wide location (e.g., `/usr/local`), but if you do, most compilers will automatically find the library, so you won't need to specify the `-I` and `-L` flags.

#### Using lightrat Python bindings

To install the Python bindings, you can use pip:

```bash
# Create a virtual environment (optional but recommended)
python3 -m venv venv
source venv/bin/activate  # Linux / macOS
venv\Scripts\activate     # Windows
# Install the bindings
pip install .
# Add the --no-build-isolation flag to reuse the existing build environment,
# Possibly speeding up the installation process.
```

You can check that the installation was successful by running the following code:

```python
# example.py

from lightrat import Rational

r = Rational(1, 2)
print(r)  # Output: 1/2
```

## Folder structure

The folder structure is as follows:

```bash
.
├── bindings                # Library bindings in other languages
│   └── pylightrat          # Python bindings
├── docs                    # Documentation
│   └── BUILD.bazel         # Documentation BUILD file
├── lightrat                # Main library source code
│   └── main.cpp
├── tools                   # Tools and scripts
│   ├── BUILD.bazel
│   ├── cpplint.bzl         # cpplint rules
│   └── rules_cc.bzl        # rules_cc rules
├── .bazelrc        # Bazel configuration file
├── .bazelversion   # Bazel version lock file
├── BUILD.bazel     # Root BUILD file
└── MODULE.bazel    # Root MODULE file
```

## Utility commands

```bash
# Build the main application.
# The executable can be found in the bazel-bin/lightrat directory
bazel build //lightrat
```

```bash
# Build the main application in debug mode
bazel build //lightrat --config=dbg
```

```bash
# Build the main application in release mode
bazel run //lightrat --config=opt
```

```bash
# Run all the tests
bazel test //tests/...
# On linux, you can also compute the code coverage (requires lcov to be installed)
bazel coverage --combined_report=lcov --config=dbg //tests/... //lightrat/...
genhtml --output genhtml "$(bazel info output_path)/_coverage/_coverage_report.dat"
```

```bash
# Lint all the code
bazel test //lightrat/...
```

```bash
# Build the documentation
# The documentation can be found in the bazel-bin/docs directory
bazel build //docs && python3 -m http.server --directory bazel-bin/docs/html
```

```bash
# Remove all the build files
bazel clean
```

```bash
# Run the depend-what-you-use (DWYU) analysis on the main application
bazel build --config=dwyu //lightrat
```

```bash
# Get some information about the cpp toolchain as bazel sees it
bazel cquery "@bazel_tools//tools/cpp:compiler" --output starlark --starlark:expr 'providers(target)'
```

```bash
# Get the dependencies of the main application.
# See https://docs.bazel.build/versions/main/query-how-to.html
bazel query --noimplicit_deps --notool_deps 'deps(//lightrat)'
```

```bash
# Run the python bindings tests for python 3.13
bazel test --config=py --python_version=3.13 //bindings/python/lightrat/tests/...
```

## Installing the Python bindings

```bash
# Install the python bindings with pip
pip install .
# By adding the --no-build-isolation flag, we can reuse the existing build environment and speed up the installation process.
# Make sure `setuptools` is installed in the current environment.
pip install --no-build-isolation .
```

```bash
# The same can be done with uv, but make sure `setuptools` is installed in the current environment.
uv pip install .
# You may still need to run the python files using python instead of uv run.
```

## Troubleshooting

### Bazel server stuck

Sometimes, especially on windows, bazel can get stuck compiling without a clear reason why.
If this happens, the bazel server may become unavailable and unable to run any other command.
To solve the situation, you can kill the bazel server and restart it.

```bash
# Get the bazel server PID.
# If it is stuck, it will return something like:
# Another command (pid=7760) is running. Waiting for it to complete on the server (server_pid=8032)...
bazel info server_pid
```

```bash
# Kill the bazel server (Windows)
taskkill /F /PID <server_pid>
# Kill the bazel server (Linux)
kill -9 <server_pid>
```

Any other command will restart the bazel server automatically.

## Implementation acknowledgements

Some implementation techniques in `lightrat::Rational` are inspired by
existing SMT solver implementations:

The compact representation with a GMP fallback is inspired by
the [OpenSMT FastRational implementation](https://verify.inf.usi.ch/opensmt), originally credited there to David
Monniaux and the Centre national de la recherche scientifique (CNRS), and by
the [Yices' GMP rational helpers](https://yices.csl.sri.com/), developed by SRI International, which is also used in the
GCD-aware arithmetic shortcuts.

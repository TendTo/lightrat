#!/usr/bin/env bash

set -o errexit -o nounset -o pipefail

# Argument provided by reusable workflow caller, see
# https://github.com/bazel-contrib/.github/blob/d197a6427c5435ac22e56e33340dff912bc9334e/.github/workflows/release_ruleset.yaml#L72
TAG=$1
# The prefix is chosen to match what GitHub generates for source archives
PREFIX="lightrat-${TAG}"
ARCHIVE="lightrat-$TAG.tar.gz"
git archive --format=tar --prefix="${PREFIX}/" "${TAG}" | gzip > "$ARCHIVE"

SHA=$(shasum -a 256 "$ARCHIVE" | awk '{print $1}')

cat << EOF
## Using Bzlmod with Bazel 7 or greater

\`\`\`starlark
# MODULE.bazel

bazel_dep(name = "lightrat", version = "${TAG}")
\`\`\`

\`\`\`starlark
# BUILD.bazel

load("@rules_cc//cc:defs.bzl", "cc_binary")
load("@rules_python//python:defs.bzl", "py_binary")

cc_binary(
    name = "example_cpp",
    srcs = ["example.cpp"],
    deps = ["@lightrat//lightrat"],
)

py_binary(
    name = "example_py",
    srcs = ["example.py"],
    main = "example.py",
    deps = ["@lightrat//bindings/python/lightrat"],
)
\`\`\`

See [the examples](https://github.com/TendTo/lightrat/tree/main/examples) or [the documentation](https://tendto.github.io/lightrat/) for additional details.

## Changelog

$(awk -v ver=$TAG '
 /^#+ \[/ { if (p) { exit }; if ($2 == "["ver"]") { p=1; next} } p && NF
' docs/Changelog.md)

EOF

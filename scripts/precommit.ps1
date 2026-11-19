bazel run --config=dbg //lightrat:lightrat_bin
bazel test --config=dbg //lightrat/...
bazel test --config=dbg //tests/...
bazel build --config=dbg --config=dwyu //lightrat
bazel test --config=py //bindings/python/lightrat/tests/...
bazel run --config=dbg //benchmarks:bench_hash
# Run examples
cd examples
bazel run //:example_cpp
bazel run '--@lightrat//tools:enable_python_build' //:example_py

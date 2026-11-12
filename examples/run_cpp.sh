#!/bin/bash
CC="g++"

echo "======================="
echo "Installing lightrat"
echo "======================="
bazel run @lightrat//lightrat:install -- --destdir=$(pwd)/build

echo "======================="
echo "Compiling dynamic library"
echo "======================="
$CC -std=c++20 -Ibuild/include example.cpp -Lbuild/lib -llightrat -o example
LD_LIBRARY_PATH=build/lib ./example # `examples` needs to find build/lib/liblightrat.so

echo "======================="
echo "Compiling static library"
echo "======================="
$CC -std=c++20 -Ibuild/include example.cpp -Lbuild/lib -llightrat -o example -static
./example # `examples` needs to find build/lib/liblightrat.so

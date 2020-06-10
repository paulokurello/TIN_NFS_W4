set -e
mkdir -p build
cd build
cmake -B ..
cmake --build .

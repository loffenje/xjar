#!/bin/bash

DEBUG_BUILD_DIR="build/debug"
RELEASE_BUILD_DIR="build/release"

usage() {
    echo "Usage: $0 [debug|release]"
    exit 1
}

if [ "$#" -ne 1 ]; then
    usage
fi

BUILD_TYPE="$1"
if [ "$BUILD_TYPE" == "debug" ]; then
    BUILD_DIR=$DEBUG_BUILD_DIR
    CMAKE_BUILD_TYPE="Debug"
elif [ "$BUILD_TYPE" == "release" ]; then
    BUILD_DIR=$RELEASE_BUILD_DIR
    CMAKE_BUILD_TYPE="Release"
else
    usage
fi

mkdir -p $BUILD_DIR

cd $BUILD_DIR || exit 1

echo "Configuring CMake project for $CMAKE_BUILD_TYPE mode..."
cmake -DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE ../..

echo "Building the project in $CMAKE_BUILD_TYPE mode..."
cmake --build . --config $CMAKE_BUILD_TYPE

echo "Build complete. Output stored in $BUILD_DIR."


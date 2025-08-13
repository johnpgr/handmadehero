#!/bin/bash

# Exit on any error
set -e

# Set project directories
PROJECT_ROOT="$(cd "$(dirname "$0")" && pwd)"
SRC_DIR="$PROJECT_ROOT/src"
BUILD_DIR="$PROJECT_ROOT/build"

# Create build directory if it doesn't exist
mkdir -p "$BUILD_DIR"

echo "Compiling application..."

# Compile the application using system SDL3
clang++ -std=c++23 \
    -Wall \
    -Wextra \
    -Wpedantic \
    -Wno-c23-extensions \
    -Wno-language-extension-token \
    $(pkg-config --cflags sdl3 2>/dev/null) \
    -o "$BUILD_DIR/main" \
    "$SRC_DIR/main.cpp" \
    $(pkg-config --libs sdl3 2>/dev/null)

echo "Build completed successfully!"
echo "Executable: $BUILD_DIR/main"

# Make the executable runnable
chmod +x "$BUILD_DIR/main"

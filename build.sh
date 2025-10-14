#!/bin/bash

# === Handmade Hero build script ===

# Exit on error
set -e

# Paths
SRC_DIR="source"
BUILD_DIR="../build"
SDL_PKG_PATH="../SDL/build"

# Ensure build directory exists
mkdir -p "$BUILD_DIR"

# Let pkg-config find SDL3
export PKG_CONFIG_PATH="$SDL_PKG_PATH:$PKG_CONFIG_PATH"

# Compile the program
echo "🔨 Building Handmade Hero..."
gcc $SRC_DIR/*.cpp -o "$BUILD_DIR/prog" $(pkg-config --cflags --libs sdl3)

echo "✅ Build complete!"
echo "Run the program with: $BUILD_DIR/prog"

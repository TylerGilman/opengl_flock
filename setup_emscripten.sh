#!/bin/bash

set -e

echo "==============================================="
echo "Emscripten SDK Setup for Flocking Simulation"
echo "==============================================="
echo ""

# Check if emsdk already exists
if [ -d "$HOME/emsdk" ]; then
    echo "✓ Emscripten SDK already installed at $HOME/emsdk"
    echo ""
    read -p "Do you want to update it? (y/n) " -n 1 -r
    echo ""
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        cd "$HOME/emsdk"
        git pull
        ./emsdk install latest
        ./emsdk activate latest
    fi
else
    echo "→ Installing Emscripten SDK to $HOME/emsdk..."
    echo ""

    cd "$HOME"
    git clone https://github.com/emscripten-core/emsdk.git
    cd emsdk

    echo ""
    echo "→ Installing latest Emscripten..."
    ./emsdk install latest

    echo ""
    echo "→ Activating Emscripten..."
    ./emsdk activate latest
fi

echo ""
echo "==============================================="
echo "✓ Emscripten Setup Complete!"
echo "==============================================="
echo ""
echo "To use Emscripten, run this command in your terminal:"
echo ""
echo "    source ~/emsdk/emsdk_env.sh"
echo ""
echo "Or add this to your ~/.bashrc for permanent activation:"
echo ""
echo "    echo 'source ~/emsdk/emsdk_env.sh' >> ~/.bashrc"
echo ""
echo "Then build the WebAssembly version:"
echo ""
echo "    cd $(pwd)"
echo "    source ~/emsdk/emsdk_env.sh"
echo "    make wasm"
echo "    make serve"
echo ""
echo "==============================================="

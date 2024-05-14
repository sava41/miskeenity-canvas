# Miskeenity Canvas

Cross platform image editor based on [dingboard.com](https://dingboard.com/).

## Requirements:
- CMake 3.28 or later
- Python 3.8 or newer
- cpp compiler

## Build on Desktop:

```bash
> cmake -B build -DCMAKE_BUILD_TYPE=<type>
> cmake --build build
```

## Build and Run WASM/HTML version via Emscripten (Linux, macOS)

Setup the emscripten SDK as described here:

https://emscripten.org/docs/getting_started/downloads.html#installation-instructions

```bash
> git clone https://github.com/emscripten-core/emsdk.git
> cd emsdk
> git pull
> ./emsdk install latest
> ./emsdk activate latest
> cd ..

> emsdk/upstream/emscripten/emcmake cmake . -B embuild -DCMAKE_BUILD_TYPE=<type>
> cmake --build embuild
```


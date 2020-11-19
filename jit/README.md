# CRISPR compiler

This repository contains the compiler for the CRISPR project.

## Prerequisites

You will need a working LLVM 8 build.

Unless you have it installed globally in you system you need to build it
from source:

```
LLVM_DIR="<wherever-you-want>"
git clone https://github.com/llvm/llvm-project.git "$LLVM_DIR"
cd "$LLVM_DIR"
git checkout release/8.x
mkdir build && cd build
cmake -DLLVM_TARGETS_TO_BUILD="X86" -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=True -DLLVM_ENABLE_PROJECTS="lld;clang" -DCMAKE_INSTALL_PREFIX="$(pwd)/install" -GNinja ../llvm
cmake --build .
cmake --install .
```

Then you'll need to tell CMake where to find it when building CRISPR by adding the option `-DCMAKE_PREFIX_PATH="$LLVM_DIR/install"`.

## Building

From this project root run

```
mkdir build && cd build
cmake ..
cmake --build .
```

Remember to add the `-DCMAKE_PREFIX_PATH` option to the first cmake invocation if you built LLVM from source.



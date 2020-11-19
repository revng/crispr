# CRISPR

CRISPR is a tool that allows easy static binary patching.
It aims to support multiple architectures and binary formats.

Currently CRISPR works with x86-64 ELF binaries.

## Building/installing

### Step 1: CRISPR compiler

Follow the instructions in `jit/README.md`

### Step 2: CRISPR patcher

Follow the instructions in `patcher/README.md`

## Usage

Say you want to patch a function `char* function_to_patch(int)` in a binary.
First, write your patch, e.g. in C:

```c
#include "stdio.h"

__attribute__((visibility("hidden")))
int preexisting_function(void);

__attribute__((visibility("protected")))
char* function_to_patch(int p) {
    printf("function_to_patch (patched): Patched version of function_to_patch called!\n");
    printf("function_to_patch (patched): Parameter value: %d\n", p);
    printf("function_to_patch (patched): preexisting_function returns: %d\n", preexisting_function());
    printf("function_to_patch (patched): Returning to the caller\n\n");
    return "A totally different string from the patched function!";
}
```

You need to set the visibility of functions that will be patched as `protected`, 
while preexisting functions need to be marked as `hidden`.

Then, compile your patch:

```bash
$ clang -S -emit-llvm patch.c -fno-exceptions -fno-unwind-tables -o patch.ll
```

You can now invoke CRISPR

```bash
$ crispr --dylib /path/to/libc.so.6 -o binary-to-patch.patched /path/to/binary-to-patch
```

If the target binary has symbols, nothing else is needed.
Otherwise, you can provide a CSV with additional symbols by using the `--symbols` option.
In this case, you would need to provide the symbol for `preexisting_function`.

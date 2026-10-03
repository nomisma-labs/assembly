# Toolchain findings

## 2026-10-03: GCC 16 module and UBSan retest

Local environment: Ubuntu 26.04.1 LTS, GCC 16.0.1 experimental snapshot 20260322
(`g++-16`, Ubuntu package `16-20260322-1ubuntu1`), Clang tools 19.1.7, CMake 4.4.3,
and Ninja 1.13.2. The targeted probes below use GCC.

The earlier GCC module/UBSan internal compiler error does not reproduce with this GCC 16
snapshot. The current example library, including `example.cppm`, builds with UBSan. The
`cmake_template_import_tests` test passes (1/1), and `hello_import` runs successfully with the
expected greetings and JSON output.

Commands from the template root, using a separate build directory:

```bash
cmake --preset dev -B build/gcc16-ubsan \
  '-DCMAKE_CXX_FLAGS=-fsanitize=undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer' \
  -DFETCHCONTENT_FULLY_DISCONNECTED=ON
cmake --build build/gcc16-ubsan --target example --parallel 2
cmake --build build/gcc16-ubsan \
  --target cmake_template_import_tests cmake_template_hello_import --parallel 2
ctest --test-dir build/gcc16-ubsan --output-on-failure \
  -R '^cmake_template_import_tests$'
build/gcc16-ubsan/bin/hello_import build/gcc16-ubsan/config/hello/settings.json
```

All commands exit successfully. This retest covers UBSan with the example wrapper module and
its consumers. The `asan` preset continues to use Clang 19 with both sanitizers.

The separate [nlohmann module retest](third_party_in_modules.md#2026-10-03-gcc-16-retest)
compiles and runs with a warning, but fails when warnings are errors.

## 2026-09-28: Historical findings

Environment: GCC 15.2.0 (toolchain PPA), Clang 19.1.1 (Ubuntu 24.04), CMake 4.4.3, Ninja 1.11.1.
The results in this section describe that earlier environment.

### Modules build with both compilers

A CMake + Ninja project with a named module, a partition, a global module fragment and an exported
template builds and runs with `g++-15` and `clang++-19`. The template's own module build (wrapper
module, native module with a partition, installed-module consumer) passes on both.

### Sanitizers: GCC 15.2 crashes on UBSan with modules

| Flags on the module interface | GCC 15.2 | Clang 19 |
|---|---|---|
| `-fsanitize=address` | builds | builds |
| `-fsanitize=undefined` | internal compiler error (`cp/module.cc`, `tree_node`) | builds |
| `-fsanitize=address,undefined -fno-sanitize-recover=undefined` | internal compiler error | builds, tests pass |

These results originally led to using Clang 19 with both sanitizers in the `asan` preset.
The GCC 16 UBSan retest above supersedes the earlier failure for its tested configuration.

### compile_commands.json and clang tools

For the same file, the two compilers record different module flags:

```
g++-15:     g++-15 -std=c++23 -fmodules-ts -fmodule-mapper=…/main.cpp.o.modmap -fdeps-format=p1689r5 … -c main.cpp
clang++-19: clang++-19 -std=c++23 @…/main.cpp.o.modmap … -c main.cpp
```

clang-tidy-19 runs cleanly over all 12 translation units of the Clang database, module interface
units and importers included, once the `clang` preset has been built (the BMIs must exist).
`tools/check_naming.sh` does exactly that.

To do (checklist item 9): clangd-19 with `--experimental-modules-support` in VS Code, and gdb on
code that imports modules.

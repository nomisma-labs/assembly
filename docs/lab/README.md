# Lab

`cmake_template` is also the place to try build and C++ features before real repos rely on them.
Experiments live in `apps/sandbox/` (`python3 tools/add_sandbox_app.py <name>` adds one); findings
go here, one file per topic, with the compiler and CMake versions, the verdict and any workaround.
Build experiments with `cmake --workflow --preset complete`; for Clang use
`cmake --preset clang -DNOM_BUILD_PRESET=complete` and `cmake --build --preset clang`.
New repos don't get this folder.

Current validation (2026-10-03) uses the versions recorded in
[Build: Toolchain](../build.md#toolchain). Fresh `dev`, `complete`, `clang`, `ci`, and `release`
builds pass all nine tests per preset; `asan` also passes all nine tests outside the sandbox.
The separate CI build with Ubuntu's archive CMake passes all nine tests. Targeted probes cover
the GCC wrapper module with UBSan and nlohmann_json in a module; detailed notes below preserve
their reproduction commands and the earlier findings dated 2026-09-28.

## Checklist

| # | Topic | Status | Where |
|---|---|---|---|
| 1 | Named modules: interfaces, partitions, implementation units, on GCC 16 and Clang 19 | Native module with a partition builds and runs in `complete`; wrapper module passes with GCC 16 and Clang 19. Implementation units not tried yet. | `apps/sandbox/native_module/`, [toolchain.md](toolchain.md) |
| 2 | Headers and modules side by side (wrapper module; `#include` and `import` users in one build) | Current GCC 16 and Clang 19 paths pass; the GCC 16 import test and `hello_import` also pass with UBSan. | `core/example/example.cppm`, `apps/hello/`, `tests/unit/example_import/`, [toolchain.md](toolchain.md) |
| 3 | Macros next to a module | Works: the macro header is included beside `import`. | `core/example/macros.h` |
| 4 | Third-party headers in the global module fragment | GCC 16 imports nlohmann 3.12 with a TU-local warning; `-Werror` fails. Outside-module workaround retained. metal not tried yet. | [third_party_in_modules.md](third_party_in_modules.md#2026-10-03-gcc-16-retest) |
| 5 | Template-heavy code through a module (relay-like typelist dispatch on metal) | To do | |
| 6 | Installed modules: consumer builds its own BMIs from an installed package | Installed-package and source consumers pass with GCC 16 and Clang 19. | `tests/integration/library_usage/` (`use_library_import`) |
| 7 | `import std;` (GCC 16 and CMake's experimental switch) | To do (optional) | |
| 8 | Inlining across module boundaries (hot path) | To do | |
| 9 | Tooling with modules: clangd, clang-tidy, gdb | Current clang-tidy check passes over module units with the Clang database; clangd and gdb to do. | [toolchain.md](toolchain.md) |
| 10 | Build time: headers vs modules | To do | |
| 11 | Dependency mechanics: submodules + FetchContent, diamonds, standalone, source override | Submodule + FetchContent and source consumption pass with GCC 16 and Clang 19. Diamond superbuild and the `FETCHCONTENT_SOURCE_DIR_<NAME>` override to do. | `tests/integration/library_usage/`, [third_party_in_modules.md](third_party_in_modules.md) |
| 12 | macOS (Homebrew LLVM) | To do | |
| 13 | C++26: pack indexing, static reflection | To do (later) | |

## Findings outside the checklist

- **GCC 16 UBSan module retest passes.** The earlier GCC 15.2 internal compiler error does not
  reproduce in the tested example wrapper module and consumers. The `asan` preset continues to
  use Clang 19. See the current and historical results in [toolchain.md](toolchain.md).
- **Importers must include the std headers they use.** A module's global module fragment doesn't
  make those declarations visible to importers: comparing `std::string`s in a file that only
  imports `nom.example` fails until it includes `<string>`.

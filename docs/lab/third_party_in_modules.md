# Third-party code with CMake and modules

Updated: 2026-10-03. The current retest and the historical findings below record their
own tested toolchains.

## nlohmann_json in a module's global module fragment

### 2026-10-03: GCC 16 retest

Local environment: Ubuntu 26.04.1 LTS, GCC 16.0.1 experimental snapshot 20260322
(`g++-16`, Ubuntu package `16-20260322-1ubuntu1`), Clang tools 19.1.7, CMake 4.4.3,
and Ninja 1.13.2. This probe uses GCC and the pinned nlohmann_json v3.12.0 submodule.

**Result:** a module including `<nlohmann/json.hpp>` in its global module fragment compiles.
Exporting an alias for `nlohmann::json` produces a `-Wexpose-global-module-tu-local` warning;
an importing consumer compiles, links, and successfully parses JSON. Compiling that module
with `-Werror` fails with `-Werror=expose-global-module-tu-local`.

The warning identifies `json_pointer::split` in `detail/json_pointer.hpp:666` exposing the
TU-local `detail::unescape` helper. The earlier hard error becomes a warning in this snapshot,
so the existing outside-module JSON mapping remains appropriate for warnings-as-errors CI.

Probe sources, created in an isolated temporary directory:

```cpp
// json.cppm
module;
#include <nlohmann/json.hpp>
export module probe.json;
export namespace probe
{
    using json = nlohmann::json;
}
```

```cpp
// main.cpp
#include <string>
import probe.json;

int main()
{
    const auto value = probe::json::parse("{\"answer\":42}");
    return value.at("answer").get<int>() == 42 ? 0 : 1;
}
```

Set `template_source` to the template checkout, then run these commands in the probe directory:

```bash
g++-16 -std=c++23 -fmodules-ts -I"$template_source/external/nlohmann_json/include" \
  -x c++ -c json.cppm -o json.o
g++-16 -std=c++23 -fmodules-ts main.cpp json.o -o json_probe
./json_probe
g++-16 -std=c++23 -fmodules-ts -Werror -I"$template_source/external/nlohmann_json/include" \
  -x c++ -c json.cppm -o json-strict.o
```

The first three commands exit 0; the final warnings-as-errors compile exits 1.

### 2026-09-28: Historical findings

Environment: GCC 15.2.0, Clang 19.1.1, CMake 4.4.3.

**Historical result:** GCC 15.2 rejects a module interface whose global module fragment includes
`<nlohmann/json.hpp>`; Clang 19.1.1 accepts it.

GCC reports `… exposes TU-local entity …`: nlohmann defines helper functions `static` in its
headers (`detail::unescape`, `detail::little_endianness`, and on the development branch also
`detail::is_valid_utf8`), and templates that the module makes reachable use them. C++20 forbids
such exposures, and GCC 15 enforces the rule.

| nlohmann version | GCC 15.2 | Clang 19.1.1 |
|---|---|---|
| v3.12.0 (latest release, April 2025) | error | builds |
| develop at `373005f` (2026-09-27) | error (fewer helpers, still `is_valid_utf8`) | not tried |

Upstream: issue [#4759](https://github.com/nlohmann/json/issues/4759) and PR
[#4764](https://github.com/nlohmann/json/pull/4764) (C++20 modules support, on develop). nlohmann's
docs also report a separate GCC bug, "redefinition" errors when `nlohmann/json.hpp` is included in
a module preamble together with other imports, unresolved as of GCC 16
([modules page](https://json.nlohmann.me/features/modules/)). The development branch also adds
its own module (`NLOHMANN_JSON_BUILD_MODULES`, `import nlohmann.json;`).

### Current template workaround

The nlohmann mapping of `settings` lives in `settings_json.h`, a
public header outside the module. `settings.h` (the type and `load_settings`) stays
nlohmann-free and is exported by `nom.example`. Files that import `nom.example` and need the JSON
mapping include `settings_json.h` before the import. nlohmann remains a public dependency, which
is the case the config repo has with `DEFINE_CONFIG`.

**For the config repo:** keep `config.h` (the `DEFINE_CONFIG` macros) a plain header; retain
this workaround until the supported compiler configuration accepts the mapping without
warnings that fail CI.

**Retest** after relevant compiler or nlohmann upgrades: update the dependency pin when needed, put
`#include <example/settings_json.h>` into `core/example/example.cppm` and export `to_json`/`from_json`,
then build and test the `dev` and warnings-as-errors `ci` presets.

## Third-party CMake projects as subprojects

The following findings were recorded on 2026-09-28 with CMake 4.4.3.

**metal** (v2.1.4): running its CMake project through `FetchContent_MakeAvailable` adds about
2,100 targets with generic names (`example`, `bundle`, `doc`), looks for Doxygen and Boost, and
prints a deprecation warning under CMake 4.4. In a superbuild those names would collide. Since metal
is header-only, use it through its include directory and never run its CMake project; for example,
declare it with a `SOURCE_SUBDIR` that has no `CMakeLists.txt`, so FetchContent only records
the sources, and add `${metal_SOURCE_DIR}/include` to the target that needs it.

**nlohmann_json** (v3.12.0): works as a subproject. When it isn't the top-level project its tests
are off, and its install rules are off too unless `JSON_Install` is set, which the template does
(so an installed package brings nlohmann along for `find_package` consumers).

Other libraries: case by case.

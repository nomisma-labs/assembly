# assembly

<!-- repo-description:begin -->
Assembly composes an application from an ordered list of C++ nodes. Nodes exchange typed
events through a bidirectional relay and contribute shared services through context mixins.
The same library provides `delegate` callback interfaces for connecting runtime event sources
to those nodes. Assembly is a module-first C++23 library: its API lives in `nom` and is exported
by `nom.assembly`. Its only library dependency is Metal v2.1.4, provided by the shared
CMake foundation as `nom::metal`.
<!-- repo-description:end -->

Read [the framework guide](docs/framework.md) for the pattern and API, then follow
[the teaching tests](docs/tests.md): `assembly`, `relay`, `complex_relay`, and `delegate`.
The complex relay suite is a small application whose events, context services, configuration,
and trait choices can be traced end to end.

Consumers link the CMake target `nom::assembly` through `find_package(assembly CONFIG REQUIRED)` or
`FetchContent`, then use `import nom.assembly;`. The installed Assembly package finds
`nom_metal`, which supplies the Metal headers and license; consumers do not need the source checkout.

Unit suites and source/installed-package consumers import `nom.assembly`;
[the test guide](docs/tests.md) explains how to run the teaching suites.

## Bootstrap

After installing the tools listed in [docs/build.md](docs/build.md#toolchain), run:

```bash
./bootstrap.sh           # standard validation presets
./bootstrap.sh release   # one preset only
./bootstrap.sh --help    # usage and available presets
```

Bootstrap discovers enabled presets through CMake, including included files and
`CMakeUserPresets.json`. It uses matching configure/build/test names and the repo's
`build/<preset>/` layout. With no argument, it validates every discovered preset except
`release`; pass a name to select one preset explicitly. Adding a paired preset to CMake
automatically makes it available here. Each run clears selected build directories and previous
bootstrap logs, initializes missing submodules, and configures/builds/tests from scratch.
Unselected build directories are preserved.

Python compatibility/imports, SPDX headers and formatting run every time. The Python check
verifies version 3.10 or newer, script syntax, and imported modules; no pip packages are
currently required. The naming check is included when `clang` is selected. Tests include unit
suites and integration checks for library usage from source and through an installed package.
Every step reports `PASS`, `FAIL`
or `SKIP`, followed by a summary and next steps. Failures return a nonzero exit code;
independent checks continue where possible. Only the current run's logs are kept under
`build/bootstrap/`. Use the commands in **Build** below for incremental work.

Submodules at
unexpected commits are reported for you to resolve; bootstrap does not reset them. Dependency
initialization or CMake's dependency fallback may need network access. Build concurrency defaults
to the available CPU core count minus one, with a minimum of one job. Use
`CMAKE_BUILD_PARALLEL_LEVEL=4 ./bootstrap.sh` to override it. The script also works when invoked
from another directory.

## Manual build

```bash
git submodule update --init --recursive   # once, after cloning
cmake --preset dev                        # once: configure into build/dev
cd build/dev
ninja                                     # build; reruns cmake itself when a CMakeLists.txt changes
ctest --output-on-failure                 # test
```

Other presets (`complete`, `clang`, `asan`, `release`, `ci`) work the same way in `build/<preset>`, and
`cmake --workflow --preset dev` configures, builds and tests in one command. Apps land in
`build/<preset>/bin/`. Use `cmake --workflow --preset complete` for a Debug build including every
app and experiment. Apps can limit their default build/install with `BUILD_FOR dev ci`; omitting
`BUILD_FOR` includes them everywhere. More: [docs/build.md](docs/build.md).

## Docs

Agents start at [AGENTS.md](AGENTS.md), people at [docs/](docs/README.md). Adding a library, an
app, tests or a dependency: [docs/howto.md](docs/howto.md). Releases: [CHANGELOG.md](CHANGELOG.md).

# AGENTS.md

<!-- repo-description:begin -->
`assembly` provides static node/context composition, selective bidirectional event routing,
and callable delegates under `nom`. The module is `nom.assembly` and the CMake target is
`nom::assembly`. It uses the shared foundation's Metal v2.1.4 headers through `nom::metal`; Catch2 is
used only for tests. Start with [docs/framework.md](docs/framework.md) and the executable
learning path in [docs/tests.md](docs/tests.md).
<!-- repo-description:end -->

## Build and test

- Run `./bootstrap.sh` for fresh validation builds, tests and quality checks, or `./bootstrap.sh <preset>` for one preset.
  Presets are discovered through CMake; the default selects all matching configure/build/test names except `release`.
  It clears selected build directories and previous bootstrap logs; `--help` lists the discovered presets.
  Python 3.10+ is required; `tools/kit/checks/python.py` validates syntax and imports before builds.
- First checkout: `git submodule update --init --recursive`.
- Configure once with `cmake --preset dev`; then, in `build/dev`, `ninja` builds and
  `ctest --output-on-failure` tests. Binaries land in `build/<preset>/bin/`.
- `cmake --workflow --preset dev` does all three. Other presets: `complete`, `clang`, `asan`, `release`, `ci`.
  `complete` is Debug with every app; `nom_app(... BUILD_FOR dev ci)` limits normal build/install participation.
  Details: [docs/build.md](docs/build.md).

## Layout

- `core/assembly/`: the `nom.assembly` module, with composition, relay and delegate interface partitions.
- `apps/<app>/`: executables; optional `config/` is collected into `build/<preset>/config/<app>/` and installed with the app.
- `apps/sandbox/`: experiments, normally `BUILD_FOR complete`; also buildable explicitly in other presets.
- `tests/unit/<suite>/` or inline `<code dir>/tests/<suite>/`: Catch2 unit tests, labeled `unit`.
  Catch2 is found at the project root when tests are enabled, before code directories.
- `tests/integration/`: library usage and installation checks, labeled `integration`.
- `external/`: git submodules (third-party code pinned to release tags, and our own repos).
- `cmake/kit/`: the shared CMake kit. `tools/kit/`: the shared checks and scripts
  ([tools/kit/README.md](tools/kit/README.md)); the repo's own tools go in `tools/`, outside the kit.
  `docs/`: documentation.

## Adding code

Follow [docs/howto.md](docs/howto.md): a library, a module, an app, a unit test suite, library usage integration tests,
a dependency, an experiment.

## Rules

- Follow [docs/conventions.md](docs/conventions.md): `snake_case` everywhere, `PascalCase` template
  parameters, UPPER_CASE only for macros. `tools/kit/checks/naming.sh` enforces the naming.
- C++ test sources use `*_test.cxx` in both unit and integration tests; regular sources use `.cpp`.
- Every source file starts with the SPDX header; `python3 tools/kit/checks/headers.py` checks it.
- Format with `tools/kit/checks/format.sh --fix`.
- Add targets with the kit (`nom_lib`, `nom_app`, `nom_c2test`), not raw CMake.
- Template-owned paths (`bootstrap.sh`, `cmake/kit/`, `tools/kit/`, `docs/build.md`, `docs/conventions.md`,
  `docs/howto.md`, `docs/new_repo.md`, `.clang-format`, `.clang-tidy`, `.clangd`, `.editorconfig`,
  `.github/workflows/ci.yml`) change only in cmake_template and reach other repos through
  `tools/kit/sync_template.py`. Don't edit them in a derived repo.
- Modules: put `#include`s before `import`s, include the std headers you use yourself, and get
  macros from headers ([docs/build.md](docs/build.md#modules)).
- The public C++ API uses namespace `nom`, overriding the generic repository namespace
  convention. Keep module name `nom.assembly` and CMake target `nom::assembly`.
- Unit suites and integration consumers import `nom.assembly`. Catch2 macros and standard-library
  declarations come from headers; Metal stays in the module units' global fragments.
  See [docs/tests.md](docs/tests.md) for the teaching suites.
- The version lives only in `project()`; releases follow [docs/build.md](docs/build.md#versioning).
- When you change behaviour that a doc describes, update the doc in the same change.

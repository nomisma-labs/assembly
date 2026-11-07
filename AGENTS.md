# AGENTS.md

<!-- repo-description:begin -->
`assembly` provides static node/context composition, selective bidirectional event routing,
and callable delegates under `nom`. The module is `nom.assembly` and the CMake target is
`nom::assembly`. It uses the Metal v2.1.4 headers through `nom::metal`; Catch2 is
used only for tests. Start with [docs/assembly/framework.md](docs/assembly/framework.md) and the executable
learning path in [docs/assembly/tests.md](docs/assembly/tests.md).
<!-- repo-description:end -->

Read and follow the rules every Nomisma repo shares: @docs/kit/agents.md

Rules for this repo alone follow below.

## This repo

- `src/assembly/`: the `nom.assembly` module, with assembly_impl, custom_traits, relay and delegate interface partitions.
- `cmake/assembly/`: this repo's own CMake files. `metal.cmake` defines `nom_find_metal()`, which
  provides `nom::metal` from the `external/metal` submodule (v2.1.4) and installs its headers and
  license as the `nom_metal` package. Link `nom::metal` where Metal headers are needed, and list
  `nom_metal` in `nom_install_package(DEPENDENCIES …)` for exported targets that use it.
- The public C++ API uses namespace `nom`, overriding the generic repository namespace
  convention. Keep module name `nom.assembly` and CMake target `nom::assembly`.
- Unit suites and integration consumers import `nom.assembly` and use `import std;` for
  standard-library declarations. Catch2 macros stay in headers; Metal stays in the module units' global fragments.
  See [docs/assembly/tests.md](docs/assembly/tests.md) for the teaching suites.

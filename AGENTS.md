# AGENTS.md

<!-- repo-description:begin -->
`assembly` provides static node/context composition, selective bidirectional event routing,
and callable delegates under `nom`. The module is `nom.assembly` and the CMake target is
`nom::assembly`. It uses the shared foundation's Metal v2.1.4 headers through `nom::metal`; Catch2 is
used only for tests. Start with [docs/framework.md](docs/framework.md) and the executable
learning path in [docs/tests.md](docs/tests.md).
<!-- repo-description:end -->

Read and follow the rules every Nomisma repo shares: @docs/kit/agents.md

Rules for this repo alone follow below.

## This repo

- `core/assembly/`: the `nom.assembly` module, with composition, relay and delegate interface partitions.
- The public C++ API uses namespace `nom`, overriding the generic repository namespace
  convention. Keep module name `nom.assembly` and CMake target `nom::assembly`.
- Unit suites and integration consumers import `nom.assembly`. Catch2 macros and standard-library
  declarations come from headers; Metal stays in the module units' global fragments.
  See [docs/tests.md](docs/tests.md) for the teaching suites.

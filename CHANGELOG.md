# Changelog

Notable changes to this repo. The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and versions follow [Semantic Versioning](https://semver.org); how to release:
[docs/kit/build.md](docs/kit/build.md#versioning).

## [Unreleased]

### Changed

- Sync the shared build infrastructure with cmake_template v0.11.1; `nom_app()` uses the
  supplied target name without a project prefix.

- Synchronized the shared build infrastructure with cmake_template v0.9.0.
  Public helpers enforce project-local build and installation options internally;
  source-consumption tests remain available when installation is disabled.

## [0.2.0] - 2026-10-13

### Added

- `nom::apply_custom_traits_t<Traits, NodeTraits>` selects an assembly's `custom_traits`
  when it inherits the node's default traits; missing or unrelated custom traits keep the defaults.
- `nom::ctx<Assembly>(part)` supports nested context mixins without additional owner aliases.
  Explicit and deduced lookup preserve constness, reject unrelated source types and access
  mixin context directly, including already constructed services during construction.
- `nom::event_suppressors` lets an assembly's custom traits list request and response events
  without a final consumer, instead of adding utility terminator nodes. The relay consumes them
  in a suppressor at the end of each route, after application handlers; unlisted events still
  require a reachable handler.
- `nom::type_list<Types...>` declares type lists without including Metal headers.
- A `custom_traits` teaching suite, and guides to reading an application through five questions,
  to event suppression and to the exception policy for event processing.

### Fixed

- Relay entry points use named routing constraints, so unsupported calls fail during overload
  resolution and can be detected with `requires`. Forwarding rejects unrelated source types
  before searching the remaining nodes, while preserving exact argument forwarding.
- Relay entry points call handlers directly after constraint checking, removing intermediate
  dispatch stack frames in debug builds and redundant checks inside the call bodies.

### Changed

- `relay<Assembly>::assembly_type` is renamed `relay<Assembly>::assembly`; the relay's `nodes`
  and `reverse_nodes` end with its request and response suppressors.
- Body and relay construction require at least one node or context mixin to accept the full
  nonempty initialization argument pack as const references. Other components may still default;
  arguments that every component would ignore are rejected by a constructor constraint.
- Synchronized the shared CMake foundation with cmake_template v0.7.0, which drops Metal from the
  kit: `cmake/assembly/metal.cmake` now provides `nom_find_metal()`. The `nom::metal` target and
  the installed `nom_metal` package are unchanged.
- Synchronized the shared tools with cmake_template v0.8.1: `./bootstrap.sh` enables git hooks
  that check each commit's staged files (whitespace, formatting, SPDX headers) and reject
  `Co-Authored-By` trailers, and sandbox experiments are git-ignored.
- Synchronized the shared CMake kit with cmake_template v0.8.2: `nom_lib()` and `nom_app()`
  accept `COMPILE_OPTIONS`.

## [0.1.0] - 2026-10-03

### Added

- Assembly/context composition, bidirectional relay, and delegate callback interfaces in `nom`.
- Four teaching suites: assembly, relay, complex_relay, and delegate.
- Native `nom.assembly` module and source/installed-package consumer checks.
- Metal v2.1.4 from the template's common dependency baseline, exposed as `nom::metal` and
  installed through the shared `nom_metal` package.

### Changed

- Public API names live directly in `nom`, including `nom::assembly`, `nom::relay`, and
  `nom::delegate`; the module remains `nom.assembly` and the CMake target remains `nom::assembly`.
- Composition, relay and delegate are exported through native module interface partitions.
- Unit suites and integration consumers import `nom.assembly`.
- Synchronized the shared CMake foundation with cmake_template v0.3.0.

### Fixed

- Relay handler detection now uses the argument categories actually passed to handlers.
- Borrowed delegates preserve reference return types and support const wrapper invocation.
- Missing relay handlers report a route assertion without an out-of-range typelist lookup.
- Construction constraints reject missing configuration; constructor-time context lookup accesses
  the completed context directly.

# Changelog

Notable changes to this repo. The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and versions follow [Semantic Versioning](https://semver.org); how to release:
[docs/build.md](docs/build.md#versioning).

## [Unreleased]

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

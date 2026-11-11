# assembly

<!-- repo-description:begin -->
Assembly composes an application from an ordered list of C++ nodes. Nodes exchange typed
events through a bidirectional relay and contribute shared services through context mixins.
The same library provides `delegate` callback interfaces for connecting runtime event sources
to those nodes. Assembly is a module-first C++23 library: its API lives in `nom` and is exported
by `nom.assembly`. Its only library dependency is Metal v2.1.4, pinned in `external/metal` and
provided as `nom::metal` by [cmake/assembly/metal.cmake](cmake/assembly/metal.cmake).
<!-- repo-description:end -->

C++ builds and consumers use CMake 4.4.x, Ninja, GCC 16 or Clang 22, and libstdc++ 16
standard-library module sources. Enable std module detection before the consumer's first
`project()` and call `nom_enable_std_modules(target)` on each compiled consumer target.
See [standard-library module setup](docs/kit/build.md#modules), including package consumers
without the kit. Macro headers remain textual includes before imports.

**Start here: [How to understand Assembly/Relay applications](docs/assembly/understanding_applications.md).**
Read the node list, then ask five questions about context services, event handlers, traits,
configuration, and event origins.

Continue with [the framework guide](docs/assembly/framework.md) for the pattern and API, then follow
[the teaching tests](docs/assembly/tests.md): `assembly`, `custom_traits`, `relay`, `complex_relay`, and `delegate`.
The complex relay suite is a small application whose events, context services, configuration,
and trait choices can be traced end to end.

Consumers link the CMake target `nom::assembly` through `find_package(assembly CONFIG REQUIRED)` or
`FetchContent`, then use `import nom.assembly;`. The installed Assembly package finds
`nom_metal`, which supplies the Metal headers and license; consumers do not need the source checkout.
Use `nom::type_list<Types...>` for type lists; it is available from `nom.assembly` without
including Metal headers.

Unit suites and source/installed-package consumers import `nom.assembly`;
[the test guide](docs/assembly/tests.md) explains how to run the teaching suites.

## Build

Install the tools listed in [docs/kit/build.md](docs/kit/build.md#toolchain), then:

```bash
./bootstrap.sh                   # fresh builds, tests and checks for every preset but release
cmake --workflow --preset dev    # configure, build and test one preset
```

Everyday commands, the presets and what bootstrap checks: [docs/kit/build.md](docs/kit/build.md).

## Docs

Agents start at [AGENTS.md](AGENTS.md), people at [docs/](docs/README.md). Adding a library, an
app, tests or a dependency: [docs/kit/howto.md](docs/kit/howto.md). Releases: [CHANGELOG.md](CHANGELOG.md).

## License

This project is licensed under [MIT](LICENSE). Copyright belongs to Alexey Tkachenko.
The synchronized build kit retains its [MIT notice](cmake/kit/LICENSE); third-party
dependencies retain their own licenses.

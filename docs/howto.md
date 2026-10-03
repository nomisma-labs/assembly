# How to add code

Recipes for the usual changes. A new repo starts empty and grows through them; cmake_template's
example code (`core/example/`, `apps/hello/`, `tests/`) is the worked reference for each one.
`<repo>` stands for the repo's name, the one in `project()`. Names follow
[conventions.md](conventions.md), every new file starts with the SPDX header
([conventions.md](conventions.md#enforcement)), and build commands are in [build.md](build.md).

## A library

The CMake project names the whole repo; library targets have their own names. A repo's main
library normally uses the repo name: target `<repo>`, alias `nom::<repo>`, namespace `nom::<repo>`.
In `cmake_template`, the illustrative library is instead named `example`: the integration test
uses `find_package(cmake_template)` and links `nom::example`.

1. Put the code in `core/<repo>/`: `<file>.h` and `<file>.cpp`. Code includes the headers as
   `<repo/file.h>`.
2. Add `core/<repo>/CMakeLists.txt`:
   ```cmake
   nom_lib(<repo>
       SOURCES file.cpp          # without SOURCES the library is header-only
       HEADERS file.h
       PUBLIC_DEPS <targets>)    # what the headers need, if anything
   if(nom_install)
       nom_install_package(TARGETS <repo> DEPENDENCIES <packages>)
   endif()
   ```

   `DEPENDENCIES` lists the packages a consumer must find first, for example `nlohmann_json`.
3. Add `add_subdirectory(<repo>)` to `core/CMakeLists.txt`.
4. Add its unit tests and library usage integration tests (below).

Use `STATIC` or `SHARED` in `nom_lib` to select compiled-library linkage explicitly.
They are mutually exclusive and require sources or modules. Omitting both preserves
the `BUILD_SHARED_LIBS` default; headers-only targets remain interface libraries.

A second library is named `<repo>_<name>`; list it in the same `nom_install_package()` call, which
a repo makes once.

## A module

The wrapper-module pattern: the headers stay the source and the module re-exports their API, so
code can `#include` the headers or `import nom.<repo>;`.

1. Add `core/<repo>/<repo>.cppm`:
   ```cpp
   module;

   #include <<repo>/file.h>

   export module nom.<repo>;

   export namespace nom::<repo>
   {
       using nom::<repo>::thing; // one line per exported name
   } // namespace nom::<repo>
   ```
2. In `core/<repo>/CMakeLists.txt`, build it only when modules are on:
   ```cmake
   set(module_files "")
   if(nom_build_modules)
       set(module_files <repo>.cppm)
   endif()
   nom_lib(<repo> … MODULES ${module_files})
   ```
3. Leave macros, and headers GCC can't take through a module (nlohmann's, for now), out of the
   module; importers include them before their imports. More rules: [build.md](build.md#modules).

## An app

1. Put `main.cpp` in `apps/<app>/`.
2. Add `apps/<app>/CMakeLists.txt`:
   ```cmake
   nom_app(<app> SOURCES main.cpp DEPS nom::<repo>)
   ```
   The target is `<repo>_<app>`; the binary is `build/<preset>/bin/<app>`. By default it joins
   every preset (`BUILD_FOR any`). Restrict its normal build and installation with:
   ```cmake
   nom_app(<app> SOURCES main.cpp DEPS nom::<repo> BUILD_FOR dev ci)
   ```
   Other presets can still build it explicitly by target name. The `complete` preset includes every
   app regardless of this list. Use `BUILD_FOR complete` for prototypes.
3. Add `add_subdirectory(<app>)` to `apps/CMakeLists.txt`.
4. Put application configuration files in `apps/<app>/config/` when needed. Building any app
   declared in that directory collects the whole folder into `build/<preset>/config/<app>/`.
   The copy runs on each build and removes stale files from the staged folder. Multiple
   executables in the same app directory share one collection target. With `nom_install` enabled and the app selected by the preset,
   `cmake --install` puts the executable in `<prefix>/bin/` and the collected files in
   `<prefix>/config/<app>/`. Build before installing; config generation will be added later.

## A unit test suite

1. Put Catch2 tests under `tests/unit/<suite>/` or inline beside the code in
   `<code dir>/tests/<suite>/`, as `<file>_test.cxx`. Test-only headers are `<name>_test.h`.
2. Give the folder a `CMakeLists.txt` with one executable:
   ```cmake
   nom_c2test(tests SOURCES file_test.cxx DEPS nom::<repo>)
   set_tests_properties(${PROJECT_NAME}_tests PROPERTIES LABELS unit)
   ```
   `nom_c2test(<suite> …)` builds `<repo>_<suite>` and registers it with CTest; each folder needs
   its own suite name (`tests`, `import_tests`, …).
3. For a top-level suite, add `add_subdirectory(<suite>)` to `tests/unit/CMakeLists.txt`. For an
   inline suite, add this to the code directory's `CMakeLists.txt`, after defining the library:
   ```cmake
   if(nom_build_tests)
       add_subdirectory(tests/<suite>)
   endif()
   ```
   A suite using `import` also needs an `if(nom_build_modules)` guard. Use unique suite names
   across both layouts. The root `CMakeLists.txt` enables testing and finds Catch2 before
   processing code directories, so both layouts can call `nom_c2test()`.

## Library usage integration tests

These verify that a separate project can find, compile against, link and use the library. They
build and run a small program twice: against the installed package through `find_package`, and
against the source tree through `FetchContent`. This checks the public API and CMake integration
that other repos rely on.

1. Add `tests/integration/library_usage/CMakeLists.txt`:
   ```cmake
   cmake_minimum_required(VERSION 3.30)
   project(<repo>_library_usage LANGUAGES CXX)

   set(CMAKE_CXX_STANDARD 23)
   set(CMAKE_CXX_STANDARD_REQUIRED ON)
   set(CMAKE_CXX_EXTENSIONS OFF)

   if(LIBRARY_USAGE_MODE STREQUAL "package")
       find_package(<repo> CONFIG REQUIRED)
   else()
       include(FetchContent)
       FetchContent_Declare(<repo> SOURCE_DIR ${LIBRARY_USAGE_SOURCE_DIR})
       FetchContent_MakeAvailable(<repo>)
   endif()

   enable_testing()
   add_executable(use_library use_library_test.cxx)
   target_link_libraries(use_library PRIVATE nom::<repo>)
   add_test(NAME use_library COMMAND use_library)
   ```
2. Add `tests/integration/library_usage/use_library_test.cxx`: a `main()` that uses the library and returns 0 when it works.
3. Add `nom_library_usage_tests(library_usage)` to `tests/integration/CMakeLists.txt`. The
   helper labels its tests `integration`. Keep other integration checks here too, with the same
   label. Run either group with `ctest --preset dev -L unit` or `ctest --preset dev -L integration`.

## A dependency

- **One of our repos:** a submodule with a relative URL, pinned to a release tag:
  ```bash
  git submodule add ../<dep>.git external/<dep>
  git -C external/<dep> checkout v<x.y.z>
  git add external/<dep>
  ```
  Then, in the top-level `CMakeLists.txt` before `add_subdirectory(core)`:
  ```cmake
  nom_require_submodule(external/<dep>)
  FetchContent_Declare(<dep> SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/<dep>)
  FetchContent_MakeAvailable(<dep>)
  ```
  Link `nom::<dep>`. If the repo's public headers use it, add `<dep>` to
  `nom_install_package(DEPENDENCIES …)`.
- **Third-party code:** a shallow submodule, pinned to a release tag:
  ```bash
  git submodule add --depth 1 <url> external/<dep>
  git config -f .gitmodules submodule.external/<dep>.shallow true
  git -C external/<dep> fetch --depth 1 origin tag v<x.y.z>
  git -C external/<dep> checkout v<x.y.z>
  git add .gitmodules external/<dep>
  ```
  Then decide case by case: the same three CMake lines when its CMake project works as a
  subproject (nlohmann_json in cmake_template), or only its include directory when it doesn't
  (metal).
- **System libraries** (Boost, OpenSSL, …): `find_package(<Package> REQUIRED)`.

## An experiment

`python3 tools/add_sandbox_app.py <name>` creates `apps/sandbox/<name>/`, built as
`bin/sandbox_<name>` and linked to the main library if the repo has one. It uses `BUILD_FOR complete`:
run `cmake --workflow --preset complete`, or build its target explicitly in another preset. If an
experiment has its own support libraries, mark them `EXCLUDE_FROM_ALL TRUE` so they build only
when a dependent app is requested. Experiments that need third-party code fetch it in their
own `CMakeLists.txt`.

## A release

See [build.md](build.md#versioning).

Installed apps locate packaged shared libraries relative to their executable directory on
Unix/macOS. Shared libraries locate sibling packaged libraries in the install library
directory. These runtime paths follow GNUInstallDirs and remain valid when the complete
install prefix is moved; system dependencies still need to be installed on the host.

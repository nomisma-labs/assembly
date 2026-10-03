# SPDX-FileCopyrightText: 2026 Alexey Tkachenko
# SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

"""Exercise executable and transitive shared-library lookup after installation moves."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile


def run(*args, **kwargs):
    subprocess.run(args, check=True, **kwargs)


def main():
    parser = argparse.ArgumentParser()
    for name in ("cmake", "compiler", "generator", "kit"):
        parser.add_argument(f"--{name}", required=True)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="nom-relocation-") as directory:
        root = Path(directory)
        source = root / "source"
        source.mkdir()
        kit = Path(args.kit).resolve().as_posix()
        (source / "CMakeLists.txt").write_text(f'''cmake_minimum_required(VERSION 3.30)
project(relocation VERSION 1.0.0 LANGUAGES CXX)
include("{kit}/nom_kit.cmake")
nom_project_options()
nom_lib(base SHARED SOURCES base.cpp)
nom_lib(middle SHARED SOURCES middle.cpp PRIVATE_DEPS base)
nom_app(check SOURCES main.cpp DEPS middle)
nom_install_package(TARGETS base middle)
''')
        (source / "base.cpp").write_text("int base() { return 42; }\n")
        (source / "middle.cpp").write_text("int base(); int middle() { return base(); }\n")
        (source / "main.cpp").write_text("int middle(); int main() { return middle() == 42 ? 0 : 1; }\n")
        build = root / "build"
        prefix = root / "prefix"
        run(args.cmake, "-S", str(source), "-B", str(build), "-G", args.generator,
            f"-DCMAKE_CXX_COMPILER={args.compiler}", "-DCMAKE_INSTALL_LIBDIR=lib/custom",
            "-DRELOCATION_BUILD_TESTS=OFF", "-DRELOCATION_BUILD_MODULES=OFF")
        run(args.cmake, "--build", str(build), "--parallel", "2")
        run(args.cmake, "--install", str(build), "--prefix", str(prefix))
        relocated = root / "relocated"
        prefix.rename(relocated)
        shutil.rmtree(build)
        env = os.environ.copy()
        for name in ("LD_LIBRARY_PATH", "DYLD_LIBRARY_PATH", "DYLD_FALLBACK_LIBRARY_PATH"):
            env.pop(name, None)
        run(str(relocated / "bin/check"), cwd=root, env=env)


if __name__ == "__main__":
    main()

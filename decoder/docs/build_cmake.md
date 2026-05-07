# OpenCSD: Building with CMake

## Build Modes

The CMake build now supports two modes:

- `OPENCSD_BUILD_MIN_LIB_STATIC=ON`
  Minimal library/static-oriented build for integration into larger environments:
  - builds `opencsd` and `opencsd_c_api`
  - builds `snapshot_parser` and `trc_pkt_lister` when `BUILD_TESTING=ON`
  - provides `install`, `uninstall`, and `clean_install` targets for the minimal library target set only

- `OPENCSD_BUILD_MIN_LIB_STATIC=OFF`
  Full project build and default mode:
  - builds shared and static libraries
  - builds the additional full project test/helper targets
  - defaults `BUILD_TESTING` to `ON`
  - installs the manpage and provides `uninstall` / `clean_install`
  - writes outputs into `decoder/lib/builddir`, `decoder/tests/lib/builddir`, and `decoder/tests/bin/builddir` when `OPENCSD_FULL_PROJECT_LAYOUT=ON`

## Recommended Build Commands

From the repository root:

The top-level `CMakeLists.txt` uses support templates from `decoder/build/cmake/`.

### Linux and Other Single-Config Generators

Minimal integration build:

```bash
cmake -S . -B build-original -G "Unix Makefiles" -DBUILD_TESTING=ON -DOPENCSD_BUILD_MIN_LIB_STATIC=ON
cmake --build build-original -j
```

Full project build:

```bash
cmake -S . -B build -G "Unix Makefiles" -DBUILD_TESTING=ON
cmake --build build -j
```

Useful variants:

```bash
cmake -S . -B build-debug -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build-debug -j
```

```bash
cmake -S . -B build-release -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build-release -j
```

Install from an out-of-source build:

```bash
cmake --install build --prefix /your/install/prefix
```

Clean only build outputs:

```bash
cmake --build build --target clean
```

Remove the entire generated build and configuration state:

```bash
rm -rf build
```

### Windows with Visual Studio 2022

Use a Visual Studio 2022 generator and pass a configuration at build time.
The helper script and the direct commands below default to `x64`.

Full project build:

```powershell
cmake -S . -B build-vs2022 -G "Visual Studio 17 2022" -A x64 -DBUILD_TESTING=ON
cmake --build build-vs2022 --config Release --parallel
```

Minimal integration build:

```powershell
cmake -S . -B build-vs2022-min -G "Visual Studio 17 2022" -A x64 -DBUILD_TESTING=ON -DOPENCSD_BUILD_MIN_LIB_STATIC=ON
cmake --build build-vs2022-min --config Release --parallel
```

Debug build:

```powershell
cmake -S . -B build-vs2022-debug -G "Visual Studio 17 2022" -A x64 -DBUILD_TESTING=ON
cmake --build build-vs2022-debug --config Debug --parallel
```

Install from an out-of-source build:

```powershell
cmake --install build-vs2022 --config Release --prefix C:\temp\opencsd-install
```

## Python Helper Script

The helper script `decoder/build/cmake/build_cmake.py` wraps configure, build,
install, uninstall, and `clean_install` flows. It can be invoked either from
the repository root or from `decoder/build/cmake/`.
If `--build-dir` is omitted, the script creates and reuses mode-specific build
directories under `decoder/build/cmake/`.

On Windows, the script defaults to the `Visual Studio 17 2022` generator,
selects the `x64` platform for Visual Studio builds, and uses the Visual Studio
bundled `cmake.exe` if `cmake` is not already on `PATH`.
On non-Windows hosts, the default generator remains `Unix Makefiles`.

### Example Commands

Configure and build a minimal library tree:

```bash
python3 decoder/build/cmake/build_cmake.py --minimal
```

Remove a build tree:

```bash
python3 decoder/build/cmake/build_cmake.py --build-dir decoder/build/cmake/build-cmake-min --clean
```

Configure and build a debug tree:

```bash
python3 decoder/build/cmake/build_cmake.py --debug
```

Configure, build, and install a minimal tree:

```bash
python3 decoder/build/cmake/build_cmake.py --minimal --install --install-prefix /tmp/opencsd-install
```

Reuse a previously configured build tree and build it without reconfiguring:

```bash
python3 decoder/build/cmake/build_cmake.py --build-dir decoder/build/cmake/build-cmake-min --no-configure
```

Install from a previous build without reconfiguring or rebuilding:

```bash
python3 decoder/build/cmake/build_cmake.py --build-dir decoder/build/cmake/build-cmake-min --no-configure --no-build --install --install-prefix /tmp/opencsd-install
```

Uninstall from a previous build without reconfiguring or rebuilding:

```bash
python3 decoder/build/cmake/build_cmake.py --build-dir decoder/build/cmake/build-cmake-min --no-configure --no-build --uninstall
```

Windows full build from a plain PowerShell prompt:

```powershell
py -3 decoder/build/cmake/build_cmake.py --build-testing ON
```

Windows debug build:

```powershell
py -3 decoder/build/cmake/build_cmake.py --debug --build-testing ON
```

Windows minimal build:

```powershell
py -3 decoder/build/cmake/build_cmake.py --minimal --build-testing ON
```

### Options

- `-h`, `--help`: Show the command help and exit.
- `--build-dir BUILD_DIR`: Use the given out-of-source build directory. If omitted, the script chooses a default under `decoder/build/cmake/`, such as `decoder/build/cmake/build-cmake`, `decoder/build/cmake/build-cmake-min`, or `decoder/build/cmake/build-cmake-min-debug`.
- `--generator GENERATOR`: Pass an explicit CMake generator. The default is platform-specific: `Visual Studio 17 2022` on Windows, `Unix Makefiles` elsewhere.
- `--platform PLATFORM`: Pass a CMake platform via `-A` for Visual Studio generators. The default is `x64` for Windows Visual Studio builds.
- `--debug`: Use a Debug build. For single-config generators this sets `-DCMAKE_BUILD_TYPE=Debug`; for multi-config generators it selects `--config Debug`.
- `--config {Debug,Release}`: Explicit build configuration. Defaults to `Release`, or `Debug` when `--debug` is set.
- `--minimal`: Configure with `-DOPENCSD_BUILD_MIN_LIB_STATIC=ON`.
- `--jobs JOBS`: Pass a parallelism level to `cmake --build --parallel`.
- `--configure-only`: Run only the CMake configure step.
- `--clean`: Remove the selected build directory before any other action. If no other action is requested, the script removes the directory and exits.
- `--no-configure`: Reuse an existing configured build tree and skip configure. The selected build directory must already contain `CMakeCache.txt`.
- `--no-build`: Skip `cmake --build`. Use this when running `--install`, `--uninstall`, or `--clean-install` against an already built tree.
- `--install`: Run `cmake --install` after configure/build, or against a previous build when combined with `--no-build`.
- `--install-prefix INSTALL_PREFIX`: Pass an installation prefix to `cmake --install`. This requires `--install`.
- `--uninstall`: Run the generated `uninstall` target after the main action.
- `--clean-install`: Run the generated `clean_install` target after the main action.
- `--build-testing {ON,OFF}`: Set `BUILD_TESTING` explicitly instead of using the project default.
- `--full-project-layout {ON,OFF}`: Set `OPENCSD_FULL_PROJECT_LAYOUT` explicitly instead of using the project default.

### Option Constraints

- `--configure-only` and `--no-configure` cannot be used together.
- `--clean` cannot be combined with `--no-configure`.
- `--clean` cannot be combined with `--no-build`.
- `--platform` is only valid with Visual Studio generators.
- `--install-prefix` requires `--install`.
- `--debug` cannot be combined with `--config Release`.
- `--no-build` cannot be combined with `--configure-only`.
- `--no-build` must be paired with one of `--install`, `--uninstall`, or `--clean-install`.

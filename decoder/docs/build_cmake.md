# OpenCSD Out-of-Source CMake Build

## Build Modes

The CMake build now supports two modes:

- `OPENCSD_BUILD_MIN_LIB_STATIC=ON`
  Minimal library/static-oriented build for integration into larger environments:
  - builds `opencsd` and `opencsd_c_api`
  - builds `snapshot_parser` and `trc_pkt_lister` when `BUILD_TESTING=ON`
  - does not provide install or uninstall targets

- `OPENCSD_BUILD_MIN_LIB_STATIC=OFF`
  Full project build and default mode:
  - builds shared and static libraries
  - builds the additional full project test/helper targets
  - defaults `BUILD_TESTING` to `ON`
  - installs the manpage and provides `uninstall` / `clean_install`
  - writes outputs into `decoder/lib/builddir`, `decoder/tests/lib/builddir`, and `decoder/tests/bin/builddir` when `OPENCSD_FULL_PROJECT_LAYOUT=ON`

## Recommended Build Commands

From the repository root:

Minimal integration build:

```bash
cd /data_nvme1n1/mleach/work/opencsd-main
cmake -S . -B build-original -G "Unix Makefiles" -DBUILD_TESTING=ON -DOPENCSD_BUILD_MIN_LIB_STATIC=ON
cmake --build build-original -j
```

Full project build:

```bash
cd /data_nvme1n1/mleach/work/opencsd-main
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

Install from the out-of-source full project build:

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

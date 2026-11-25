# Repository Guidelines

## Project Structure & Module Organization
This repository is centered on `decoder/`. Public headers live in `decoder/include/opencsd`, core implementation in `decoder/source`, and platform build entry points in `decoder/build/linux`, `decoder/build/macos`, and `decoder/build/win-vs2022`. Contributor-facing docs are in `decoder/docs` plus the root `README.md` and `HOWTO.md`. Tests and sample utilities live in `decoder/tests`: sources in `decoder/tests/source`, snapshot fixtures in `decoder/tests/snapshots` and `decoder/tests/snapshots-ete`, scripts in `decoder/tests/run_*.bash`, and build outputs in `decoder/tests/bin` and `decoder/tests/lib`.

## Build, Test, and Development Commands
Use the platform makefiles from the repo root:

- `make -C decoder/build/linux` builds shared/static libraries and test tools into `decoder/lib/builddir` and `decoder/tests/bin/builddir`.
- `make -C decoder/build/linux -f makefile.dev` builds a development layout such as `decoder/tests/bin/linux64/rel`, which matches the bundled test scripts.
- `make -C decoder/build/linux DEBUG=1` enables a debug build.
- `make -C decoder/build/linux docs` runs Doxygen and writes HTML to `decoder/docs/html`.
- `bash decoder/tests/run_pkt_decode_tests.bash` runs the main snapshot decode sweep.
- `bash decoder/tests/run_pkt_decode_tests-ete.bash` runs the ETE snapshot sweep.
- On Windows, build `decoder/build/win-vs2022/ref_trace_decode_lib/ref_trace_decode_lib.sln`.

## Coding Style & Naming Conventions
The codebase is C and C++11 with GNU/Clang makefiles. There is no repository formatter config, so preserve local style instead of reformatting files wholesale. Match nearby files: 4-space indentation in C/C++, tabs where makefiles require them, braces on their own lines, and existing header comment blocks. Follow established lowercase, underscore-separated filenames such as `trc_pkt_decode_etmv4i.cpp`, `ocsd_c_api.cpp`, and `mem_acc_test.cpp`.
All edits must appear before the end of file comments where present.

## Testing Guidelines
Coverage is snapshot-driven rather than percentage-gated. Add or update fixtures under `decoder/tests/snapshots*` when decoder behavior changes, then run the relevant `run_pkt_decode_tests*.bash` script. For focused checks, run `decoder/tests/bin/<target>/trc_pkt_lister -ss_dir <snapshot> -decode` or `c_api_pkt_print_test -ss_path ./decoder/tests/snapshots -decode`.

## Commit & Pull Request Guidelines
Recent history uses short, scoped subjects such as `opencsd: etmv4: Fix ...` and `tests: add test for FEAT_CMPBR`. Keep commits narrowly focused, prefix by area (`opencsd:`, `tests:`, `docs:`), and reference GitHub issues when applicable. PRs should state the affected decoder or tool, list the platforms exercised, and note which snapshot scripts or manual commands were run. Include sample logs only when output behavior changes materially.

## Environment & Configuration Tips
Do not build from a checkout path containing spaces; the Unix makefiles document that as unsupported. When validating runtime behavior, use the existing environment toggles instead of ad hoc patches, for example `OPENCSD_INSTR_RANGE_LIMIT`, `OPENCSD_ERR_ON_AA64_BAD_OPCODE`, and the `OPENCSD_MEMACC_CACHE_*` settings.

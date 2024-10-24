Building and using the Library   {#build_lib}
==============================

@brief How to build the library and test programs and include the library in an application

Platform Support
----------------

The current makefiles and build projects support building the library on:
 - Linux and Windows, x86 or x64 hosts.
 - ARM linux - AArch32 and AArch64
 - ARM aarch32 and aarch64 libs, x-compiled on x86/64 hosts.
 - MacOS - x64 and AArch64 (Apple Silicon)

In addition to building the library from the project, the library may be installed into the standard
`/usr/lib/` area in Linux, and will soon be available as a package from Linux Distros.

Building the Library
--------------------

The library and test programs are built from the library `./build/<platform>` directory, where
<platform> is either 'linux', 'macos', or 'win-vs2022'

See [`./docs/test_progs.md`](@ref test_progs) for further information on use of the test 
programs.

### Linux x86/x64/ARM ###

GCC is assumed to be the default compiler.

Libraries are built into a <tgt_dir>. This is used as the final output directory for the
libraries in `decoder/lib/<tgt_dir>`, and also as a sub-directory of the build process for
intermediate files - `decoder/build/linux/ref_trace_decode_lib/<tgt_dir>`.

For a standard build, go to the `./build/linux/` and run `make` in that directory.

This will set <tgt_dir> to `builddir` for all build variants of the library. Using this only one variant of the library can be built at any one time.

For development, alternatively use `make -f makefile.dev`

This will set <tgt_dir> to `linux<bit-variant>/<dbg|rel>` and therefore build libraries into the
`decoder/lib/linux<bit-variant>/<dbg|rel>` directories, allowing multiple variants of the library
to be present during development.

e.g.

`./lib/linux64/rel` will contain the linux 64 bit release libraries.

`./lib/linux-arm64/dbg` will contain the linux aarch 64 debug libraries for ARM.

Options to pass to both makefiles are:-
- `DEBUG=1`   : build the debug version of the library.

Options to pass to makefile.dev are:-
- ARCH=<arch> : sets the bit variant in the delivery directories. Set if cross compilation for ARCH
                other than host. Otherwise ARCH is auto-detected.
                <arch> can be x86, x86_64, arm, arm64, aarch64, aarch32

For cross compilation, set the environment variable `CROSS_COMPILE` to the name path/prefix for the
compiler to use. The following would set the environment to cross-compile for ARM

         export PATH=$PATH:~/work/gcc-x-aarch64-6.2/bin
         export ARCH=arm64
         export CROSS_COMPILE=aarch64-linux-gnu-

The makefile will scan the `ocsd_if_version.h` to get the library version numbers and use these
in the form Major.minor.patch when naming the output .so files.

Main C++ library names:
- `libcstraced.so.M.m.p` : shared library containing the main C++ based decoder library
- `libcstrace.so.M`      : symbolic link name to library - major version only.
- `libcstrace.so`        : symbolic link name to library - no version.

C API wrapper library names:
- `libcstraced_c_api.so.M.m.p` : shared library containing the C-API wrapper library. Dependent on `libcstraced.so.M`
- `libcstraced_c_api.so.M`     : symbolic link name to library - major version only.
- `libcstraced_c_api.so`       : symbolic link name to library - no version.

Static versions of the libraries:
- `libcstraced.a`        : static library containing the main C++ based decoder library.
- `libcstraced_c_api.a`  : static library containing the C-API wrapper library.

Test programs are delivered to the `./tests/bin/<tgt_dir>` directories.

The test programs are built to used the .so versions of the libraries. 
-  `trc_pkt_lister`         - dependent on `libcstraced.so`.
-  `simple_pkt_print_c_api` - dependent on `libcstraced_c_api.so` & hence `libcstraced.so`.

The test program build for `trc_pkt_lister` also builds an auxiliary library used by this program for test purposes only.
This is the `libsnapshot_parser.a` library, delivered to the `./tests/lib/<tgt_dir>` directories.

**Note on Linux Build Directory Names**

Due to tool limitations, the makefiles will not operate correctly if the path to the opencsd directories contains spaces.

e.g. checking out the project into a directory such as ` /home/name/my opencsd/` will result in build failures.

__Installing on Linux__

The libraries can be installed on linux using the `make install` command. This will usually require root privileges. Installation will be the version in the `./lib/<tgt_dir>` directory, according to options chosen.

e.g.  ` make -f makefile.dev DEBUG=1 install`

will install from `./lib/linux64/dbg`

The libraries `libopencsd` and `libopencsd_c_api` are installed to `/usr/lib`.

Sufficient header files to build using the C-API library will be installed to `/usr/include/opencsd`.

The installation can be removed using `make clean_install`. No additional options are necessary. 


### MacOS x64/AArch64 ###

Clang is assumed to be the default compiler.

Go to the `./decoder/build/macos/` directory and run `make`.

Output libraries will be placed in the same folders as for the Linux build (`decoder/lib/builddir/`
and `decoder/tests/lib/builddir/`).

`DYLD_LIBRARY_PATH` will need to be specified to run an executable that depends on the libraries. For example,
to run `trc_pkt_lister` from the base directory:

         DYLD_LIBRARY_PATH=./decoder/lib/builddir ./decoder/tests/bin/builddir/trc_pkt_lister -h

For development, alternatively use `make -f makefile.dev`

Similar to the Linux makefile.dev, this will build libraries into the `decoder/lib/darwin<bit-variant>/<dbg|rel>`
directories, allowing multiple variants of the library to be present during development.

Options to pass to both makefiles are:-
- `DEBUG=1`   : build the debug version of the library.

Options to pass to makefile.dev are:-
- ARCH=<arch> : Set this if needing to build for x86_64 from an arm64 host, or to build for arm64 from an x86_64 host.
                Otherwise ARCH is auto-detected.
                <arch> can be x86_64, arm64


### Windows (x86/x64)  ###

Use the `.\build\win\ref_trace_decode_lib\ref_trace_decode_lib.sln` file to load a solution
which contains all library and test build projects.

Libraries are delivered to the `./lib/win<bitsize>/<dbg\rel>` directories.
e.g. `./lib/win64/rel` will contain the windows 64 bit release libraries.

The solution contains four configurations:-
- *Debug* : builds debug versions of static C++ main library and C-API libraries, test programs linked to the static library.
- *Debug-dll* : builds debug versions of static main library and C-API DLL. C-API statically linked to the main library. 
C-API test built as `c_api_pkt_print_test_dll.exe` and linked against the DLL version of the C-API library.
- *Release* : builds release static library versions, test programs linked to static libraries.
- *Release-dll* : builds release C-API DLL, static main library.

_Note_: Currently there is no Windows DLL version of the main C++ library. This may follow once
the project is nearer completion with further decode protocols, and the classes requiring export are established..

Libraries built are:-
- `libopencsd.lib` : static main C++ decoder library.
- `libopencsd_c_api.lib` : C-API static library. 
- `libopencsd_c_api.dll` : C-API DLL library. Statically linked against `libcstraced.lib` at .DLL build time. Built using the release-dll or debug-dll solution configurations.


There is also a project file to build an auxiliary library used `trc_pkt_lister` for test purposes only.
This is the `snapshot_parser_lib.lib` library, delivered to the `./tests/lib/win<bitsize>/<dgb\rel>` directories.


### Additional Build Options ###

__Library Virtual Address Size__

The ocsd_if_types.h file includes a #define that controls the size of the virtual addresses 
used within the library. By default this is a 64 bit `uint64_t` value.

When building for ARM architectures that have only a 32 bit Virtual Address, and building on 
32 bit ARM architectures it may be desirable to build a library that uses a v-addr size of 
32 bits. Define `USE_32BIT_V_ADDR` to enable this option


Including the Library in an Application
---------------------------------------

The user source code includes a header according to the API to be used:-

- Main C++ decoder library - include `opencsd.h`. Link to C++ library. 
- C-API library - include `opencsd_c_api.h`. Link to C-API library.

### Linux build ###

By default linux builds will link against the .so versions of the library. Using the C-API library will also
introduce a dependency on the main C++ decoder .so. Ensure that the library paths and link commands are part of the 
application makefile.

To use the static versions use appropriate linker options.

### MacOS build ###

The same applies as for the Linux build w.r.t the .dylib versions of the library.

### Windows build ###

To link against the C-API DLL, include the .DLL name as a dependency in the application project options.

To link against the C-API static library, include the library name in the dependency list, and define the macro 
`OCSD_USE_STATIC_C_API` in the preprocessor definitions. This ensures that the correct static bindings are declared in
the header file. Also link against the main C++ library.

To link against the main C++ library include the library name in the dependency list.


Library Performance Options
---------------------------

The library caches parts of the memory images requested during the decode process, to improve performance by reducing the number of requests to the memory accessor (memacc) objects or callbacks.

The default settings can be adjusted at runtime either by programmable API, or using environment variables.

Cache parameters can be set in terms of page size and number of pages.
Caching can also be disabled.

Page size can vary between 64 bytes and 16384 bytes.
Number of pages can vary between 4 and 256.

Default values are set at 16 pages of 2048 bytes.

### Environment variables to control caching ###

- `OPENCSD_MEMACC_CACHE_PAGE_SIZE` : Page size in bytes.
- `OPENCSD_MEMACC_CACHE_PAGE_NUM`  : number of pages.
- `OPENCSD_MEMACC_CACHE_OFF`       : disable memacc caching.


Library Debug Options
---------------------

### ETMv4 / ETE instruction run limit ###

The ETMv4 / ETE decoder has an optional run length limit for the amount of instructions in a range permitted before an error code will be returned.

This option allows debug of potential runaway decode if incorrect memory image imformation is provided to the debuger.

Option controlled by environment variable `OPENCSD_INSTR_RANGE_LIMIT`. Set value to number of instructions as the limit. 

### AA64 Invalid opcode detection ###

The instruction decode part of the library can be set to detect invalid aarch64 opcodes and throw an error.

In the aarch64 opcodes define any opcode with the top 16 bits as 0x0000 as an invalid opcode range - which is the range detected by the library.
Any other opcodes that are undefined or invalid that are not in this range will not be detected.

This also allows the potential to detect runaway decode where incorrect memory information is supplied which contains data sections initilised with 0x00000000.

Option is controlled by environment variable `OPENCSD_ERR_ON_AA64_BAD_OPCODE`. If this is set then the invalid opcodes will be detected.


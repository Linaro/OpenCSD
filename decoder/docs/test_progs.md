Test Programs    {#test_progs}
=============

@brief A description of the test programs used with the library.

The Programs
------------

There are currently a number test programs built alongside the library.

__Principle Test Programs__

- `trc_pkt_lister`:

This tests the C++ library by taking a trace "snapshot" directory as an input and decodes all
or a chosen set of trace sources from within the trace data buffers in the library. Command
line parameters allow the test program to be controlled.

Users may also run this program on their own trace snapshots to investigate or validate trace from their platforms.

The program may be installed alongside the library onto linux systems, using `make install` from the library build
directories. This will be installed alongside a `man` file containing relevant user options.

- `c_api_pkt_print_test`:

This program tests the "C" API functions, using hardcoded tests based on the same "snapshots" used
for the C++ library. Limited user control for this program.

This can also run tests using the external test decoder library to validate the external decoder API. 
See [external_custom.md](@ref custom_decoders) for details.

__Development Utilities__

These are small utilities, primarily used during the development and test of the decoder library.

- `mem-acc-test`           : tests the memory accessor interfaces.
- `mem-buffer-eg`          : example using a memory buffer input to the library.
- `frame-demux-test`       : tests the library CoreSight Frame demux object.
- `ocsd-perr`              : quickly list the library error codes and descriptions.

__Build and Install__

All the test programs are built at the same time as the library for the same set of platforms.
See [build_libs.md](@ref build_lib) for build details.

Only `trc_pkt_lister` will be installed alongside the library.


Trace "Snapshot" directory.
----------------------------

The `.\tests\snapshots` and `.\tests\snapshots-ete` directories contain a number of trace snapshots
used for testing the library.

Trace snapshots are dumps of captured binary trace data, CoreSight component configurations and memory 
dumps to allow trace decode.

Snapshots are generated on ARM targets and can then be analysed offline. 

__Snapshot Specification__

The principal snapshot format is available in a separate document in
`.\docs\specs\ARM Trace and Debug Snapshot file format 0v2.pdf`.

The programs above use the library's [core name mapper helper class] (@ref CoreArchProfileMap) to map 
the name of the core into a profile / architecture pair that the library can use. 

The snapshot definition must use one of the names recognised by this class, or alternatively use an approved
profile / architecture profile pattern string as defined below.

There are extensions to this specification, reflecting recent architectural changes.

**Dump File Section Space Format**

The dump file section in device .ini files can define a memory space associated with the file.
This is done using the `space` keyword. Omitting this keyword with cause the test programs to assume 
that the file applies to all memory spaces enccountered in the trace stream

For complex systems, the same virtual addresses may have differing contents in differing memory spaces.
The library has extended the memory space names defined in the current specification version to include 
new names for the Realm and Root memory spaces.

Mappings of names to spaces is used as follows :
    - `EL1S` : maps file to EL1 / EL0 secure states.
    - `EL2S` : maps file to EL2 secure state.
    - `EL3`  : maps file to EL3 secure state.
    - `EL1N` : maps file to EL1 / EL0 non-secure state.
    - `EL2` or `EL2N` : maps file to EL2 non-secure state.
    - `EL1R` : maps file to EL1 / EL0 Realm state.
    - `EL2R` : maps file to EL2 Realm state.
    - `ROOT` : maps file to Root state.
    - `S` : maps file to all secure states.
    - `N` : maps file to all non-secure states.
    - `R` : maps file to all Realm states.
    - `ANY` : maps file to all security states. This is default if the `space` keyword is omitted.


e.g. - Dump section examples with differing memory space definitions.

   - dump 1 & 2 overlap in address but are in different memory spaces.
   - dump 1 & 3 cover the same memory space, but do not overlap in address.
   - dump 4 covers all memory spaces but does not overlap in address with any of the other dumps.

~~~~~~~~~~~~~~~~
[dump1]
file=bindir_64ns/OTHERS_exec
address=0x00060000
length=0x21388
space=N

[dump2]
file=bindir_64rt/OTHERS_exec
address=0x00060000
length=0x21388
space=ROOT

[dump3]
file=bindir_64ns/VAL_NON_DET_CODE_exec
address=0x00010000
length=0x24bf4
space=EL1N

[dump4]
file=bindir_64ns/TEST_NON_DET_CODE_exec
address=0x00050000
length=0x26c
~~~~~~~~~~~~~~~~

**Profile / Architecture pattern string**

Where a specific core name is not used - then a profile / architecture pattern string may be used.
This enables trace generated on cores with names not in the library to be decoded by the test programs.

Pattern strings can be of the form:

`ARMvM[.m]-P` : 
    - ARMv   : fixed prefix
    - M      : architecture major version number 7-9.
    - .m     : optional minor version number
    - -P     : profile type, one of -A, -R or -M


e.g. `ARMv8.3-A`  , `ARMv7-M`

This format can be used for any ARMv7 / ARMv8 core - including ARM Cortex cores where the name is 
not one of those mapped in the library.


`ARM-{aa|AA}64[-P]` :
    - ARM-   : fixed prefix
    - aa64 or AA64 : indicator for aarch64
    - -P : optional profile - one of -R or -M, if missing A profile is assumed.

e.g. `ARM-aa64` , `ARM-AA64-R`

This format can be used for all Arm v9 architecture cores.


The `trc_pkt_lister` program.
-----------------------------

This will take a snapshot directory as an input, and lists and/or
decodes all the trace packets from a given trace sink, for any source in
that sink where the protocol is supported.

The output will be a list of discrete packets, generic output packets and any error messages
to file and/or screen as selected by the input command line options.

By default the program will list packets only (no decode), for the first discovered trace sink
(ETB, ETF, ETR) in the snapshot directory, with all source streams output.

__Command Line Options__

*Snapshot selection*

- `-ss_dir <dir>` : Set the directory path to a trace snapshot.
- `-ss_verbose`   : Verbose output when reading the snapshot.

*Decode options*

- `-id <n>`          : Set an ID to list (may be used multiple times) - default if no id set is for all IDs to be printed.
- `-src_name <name>` : List packets from a given snapshot source name - e.g ETB_0. (defaults to first source found).
- `-multi_session`   : Decode all buffers listed in snapshot under `buffers` key in `trace.ini`. Uses config of first 
                       buffer to decode all. Ignored if `-src_name` is used.
- `-dstream_format`  : Input is DSTREAM framed.
- `-tpiu`            : Input data is from a TPIU source that has TPIU FSYNC packets present.
- `-tpiu_hsync`      : Input data is from a TPIU source that has both TPIU FSYNC and HSYNC packets present.
- `-decode`          : Full decode of the packets from the trace snapshot (default is to list undecoded packets only.
- `-decode_only`     : Does not list the undecoded packets, just the trace decode.
- `-src_addr_n`      : ETE protocol; Indicate skipped N atoms in source address packet ranges by breaking the decode 
                       range into multiple ranges of N atoms.
- `-o_raw_packed`    : Output raw packed trace frames.
- `-o_raw_unpacked`  : Output raw unpacked trace data per ID.
- `-stats`           : Output packet processing statistics (if available).

*Consistency Checks*

- `-aa64_opcode_chk` : Check for correct AA64 opcodes (MSW != 0x0000)
- `-direct_br_cond`  : Check for N atoms on unconditional direct branches.
- `-strict_br_cond`  : Check for N atoms on all unconditional branches.
- `-range_cont`      : Check for address consistency between ranges after none taken branches.
- `-halt_err`        : Halt on packet/image errors - default is to reset and attempt to recover.

*Output options*

Default is to output to file and stdout. Setting any option overrides and limits to only
the options set.
- `-logstdout`          : output to stdout.
- `-logstderr`          : output to stderr.
- `-logfile`            : output to file using the default log file name.
- `-logfilename <name>` : change the name of the output log file.

*Library Development options*

Options that are only useful if developing or testing the OpenCSD library.

- `-test_waits <N>`  : Force wait from packet printer for N packets - test the wait/flush mechanisms for the decoder.
- `-profile`         : Mute logging output while profiling library performance.
- `-macc_cache_disable` : Switch off caching on memory accessor.
- `-macc_cache_p_size`  : Set size of caching pages.
- `-macc_cache_p_num`   : Set number of caching pages.

__Test output examples__

Example command lines with short output excerpts.

*TC2, ETMv3 packet processor output, raw packet output.*

Command line:-
`trc_pkt_lister -ss_dir ..\..\..\snapshots\TC2 -o_raw_unpacked`

~~~~~~~~~~~~~~~~
Frame Data; Index  17958; ID_DATA[0x11]; 16 04 c0 86 42 97 e1 c4 
Idx:17945; ID:11;	I_SYNC : Instruction Packet synchronisation.; (Periodic); Addr=0xc00416e2; S;  ISA=Thumb2; 
Idx:17961; ID:11;	P_HDR : Atom P-header.; WEN; Cycles=1
Frame Data; Index  17968; ID_DATA[0x11]; ce af 90 80 80 00 a4 84 a0 84 a4 88 
Idx:17962; ID:11;	TIMESTAMP : Timestamp Value.; TS=0x82f9d13097 (562536984727) 
Idx:17974; ID:11;	P_HDR : Atom P-header.; WW; Cycles=2
Idx:17975; ID:11;	P_HDR : Atom P-header.; WE; Cycles=1
Idx:17976; ID:11;	P_HDR : Atom P-header.; W; Cycles=1
Idx:17977; ID:11;	P_HDR : Atom P-header.; WE; Cycles=1
Idx:17978; ID:11;	P_HDR : Atom P-header.; WW; Cycles=2
Idx:17979; ID:11;	P_HDR : Atom P-header.; WEWE; Cycles=2
Frame Data; Index  17980; ID_DATA[0x10]; a0 82 
Idx:17980; ID:10;	P_HDR : Atom P-header.; W; Cycles=1
Idx:17981; ID:10;	P_HDR : Atom P-header.; WEE; Cycles=1
Frame Data; Index  17984; ID_DATA[0x10]; b8 84 a4 88 a0 82 
Idx:17984; ID:10;	P_HDR : Atom P-header.; WWWWWWW; Cycles=7
Idx:17985; ID:10;	P_HDR : Atom P-header.; WE; Cycles=1
Idx:17986; ID:10;	P_HDR : Atom P-header.; WW; Cycles=2
Idx:17987; ID:10;	P_HDR : Atom P-header.; WEWE; Cycles=2
Idx:17988; ID:10;	P_HDR : Atom P-header.; W; Cycles=1
Idx:17989; ID:10;	P_HDR : Atom P-header.; WEE; Cycles=1
~~~~~~~~~~~~~~~~

*Juno - ETB_1 selected which contains STM source output, raw packet output*

Command line:-
`trc_pkt_lister -ss_dir ..\..\..\snapshots\juno_r1_1 -o_raw_unpacked -src_name ETB_1`

~~~~~~~~~~~~~~~~
Trace Packet Lister: CS Decode library testing
-----------------------------------------------

Trace Packet Lister : reading snapshot from path ..\..\..\snapshots\juno_r1_1
Using ETB_1 as trace source
Trace Packet Lister : STM Protocol on Trace ID 0x20
Frame Data; Index      0; ID_DATA[0x20]; ff ff ff ff ff ff ff ff ff ff 0f 0f 30 41 
Idx:0; ID:20;	ASYNC:Alignment synchronisation packet.
Idx:11; ID:20;	VERSION:Version packet.; Ver=3
Frame Data; Index     16; ID_DATA[0x20]; f1 1a 00 00 00 30 10 af 01 00 00 10 03 f2 1a 
Idx:13; ID:20;	M8:Set current master.; Master=0x41
Idx:17; ID:20;	D32M:32 bit data; with marker.; Data=0x10000000
Idx:22; ID:20;	C8:Set current channel.; Chan=0x0001
Idx:23; ID:20;	D32M:32 bit data; with marker.; Data=0x10000001
Idx:28; ID:20;	C8:Set current channel.; Chan=0x0002
Frame Data; Index     32; ID_DATA[0x20]; 00 00 00 32 30 af 01 00 00 30 03 f4 1a 00 00 
Idx:30; ID:20;	D32M:32 bit data; with marker.; Data=0x10000002
Idx:36; ID:20;	C8:Set current channel.; Chan=0x0003
Idx:37; ID:20;	D32M:32 bit data; with marker.; Data=0x10000003
Idx:42; ID:20;	C8:Set current channel.; Chan=0x0004
Frame Data; Index     48; ID_DATA[0x20]; 00 f4 ff ff ff ff ff ff ff ff ff ff f0 00 13 
Idx:44; ID:20;	D32M:32 bit data; with marker.; Data=0x10000004
Idx:50; ID:20;	ASYNC:Alignment synchronisation packet.
Idx:61; ID:20;	VERSION:Version packet.; Ver=3
~~~~~~~~~~~~~~~~

*Juno - ETMv4 full trace decode + packet monitor, source trace ID 0x10 only.*

Command line:-
`trc_pkt_lister -ss_dir ..\..\..\snapshots\juno_r1_1 -decode -id 0x10`

~~~~~~~~~~~~~~~~

Idx:17204; ID:10; [0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x80 ];	I_ASYNC : Alignment Synchronisation.
Idx:17218; ID:10; [0x01 0x01 0x00 ];	I_TRACE_INFO : Trace Info.; INFO=0x0
Idx:17221; ID:10; [0x9d 0x00 0x35 0x09 0x00 0xc0 0xff 0xff 0xff ];	I_ADDR_L_64IS0 : Address, Long, 64 bit, IS0.; Addr=0xFFFFFFC000096A00; 
Idx:17230; ID:10; [0x04 ];	I_TRACE_ON : Trace On.
Idx:17232; ID:10; [0x85 0x00 0x35 0x09 0x00 0xc0 0xff 0xff 0xff 0xf1 0x00 0x00 0x00 0x00 0x00 ];	I_ADDR_CTXT_L_64IS0 : Address & Context, Long, 64 bit, IS0.; Addr=0xFFFFFFC000096A00; Ctxt: AArch64,EL1, NS; CID=0x00000000; VMID=0x0000;
Idx:17248; ID:10; [0xf7 ];	I_ATOM_F1 : Atom format 1.; E
Idx:17230; ID:10; OCSD_GEN_TRC_ELEM_TRACE_ON( [begin or filter])
Idx:17232; ID:10; OCSD_GEN_TRC_ELEM_PE_CONTEXT((ISA=A64) EL1N; 64-bit; VMID=0x0; CTXTID=0x0; )
Idx:17248; ID:10; OCSD_GEN_TRC_ELEM_INSTR_RANGE(exec range=0xffffffc000096a00:[0xffffffc000096a10] num_i(4) last_sz(4) (ISA=A64) E ISB )
Idx:17249; ID:10; [0x9d 0x30 0x25 0x59 0x00 0xc0 0xff 0xff 0xff ];	I_ADDR_L_64IS0 : Address, Long, 64 bit, IS0.; Addr=0xFFFFFFC000594AC0; 
Idx:17258; ID:10; [0xf7 ];	I_ATOM_F1 : Atom format 1.; E
Idx:17258; ID:10; OCSD_GEN_TRC_ELEM_ADDR_NACC( 0xffffffc000594ac0 )
Idx:17259; ID:10; [0x95 0xd6 0x95 ];	I_ADDR_S_IS0 : Address, Short, IS0.; Addr=0xFFFFFFC000592B58 ~[0x12B58]
Idx:17262; ID:10; [0xf9 ];	I_ATOM_F3 : Atom format 3.; ENN
Idx:17262; ID:10; OCSD_GEN_TRC_ELEM_ADDR_NACC( 0xffffffc000592b58 )
Idx:17264; ID:10; [0xf7 ];	I_ATOM_F1 : Atom format 1.; E
Idx:17265; ID:10; [0x9a 0x32 0x62 0x5a 0x00 ];	I_ADDR_L_32IS0 : Address, Long, 32 bit, IS0.; Addr=0xFFFFFFC0005AC4C8; 
Idx:17270; ID:10; [0xdb ];	I_ATOM_F2 : Atom format 2.; EE
Idx:17270; ID:10; OCSD_GEN_TRC_ELEM_ADDR_NACC( 0xffffffc0005ac4c8 )
Idx:17271; ID:10; [0x9a 0x62 0x52 0x0e 0x00 ];	I_ADDR_L_32IS0 : Address, Long, 32 bit, IS0.; Addr=0xFFFFFFC0000EA588; 
Idx:17276; ID:10; [0xfc ];	I_ATOM_F3 : Atom format 3.; NNE
Idx:17276; ID:10; OCSD_GEN_TRC_ELEM_ADDR_NACC( 0xffffffc0000ea588 )
Idx:17277; ID:10; [0x9a 0x58 0x15 0x59 0x00 ];	I_ADDR_L_32IS0 : Address, Long, 32 bit, IS0.; Addr=0xFFFFFFC000592B60; 
Idx:17283; ID:10; [0x06 0x1d ];	I_EXCEPT : Exception.;  IRQ; Ret Addr Follows;
Idx:17285; ID:10; [0x95 0x59 ];	I_ADDR_S_IS0 : Address, Short, IS0.; Addr=0xFFFFFFC000592B64 ~[0x164]
Idx:17283; ID:10; OCSD_GEN_TRC_ELEM_ADDR_NACC( 0xffffffc000592b60 )
Idx:17283; ID:10; OCSD_GEN_TRC_ELEM_EXCEPTION(pref ret addr:0xffffffc000592b64; excep num (0x0e) )
Idx:17287; ID:10; [0x9a 0x20 0x19 0x08 0x00 ];	I_ADDR_L_32IS0 : Address, Long, 32 bit, IS0.; Addr=0xFFFFFFC000083280; 
Idx:17292; ID:10; [0xfd ];	I_ATOM_F3 : Atom format 3.; ENE
Idx:17292; ID:10; OCSD_GEN_TRC_ELEM_INSTR_RANGE(exec range=0xffffffc000083280:[0xffffffc000083284] num_i(1) last_sz(4) (ISA=A64) E BR  )
Idx:17292; ID:10; OCSD_GEN_TRC_ELEM_INSTR_RANGE(exec range=0xffffffc000083d40:[0xffffffc000083d9c] num_i(23) last_sz(4) (ISA=A64) N BR   <cond>)
Idx:17292; ID:10; OCSD_GEN_TRC_ELEM_INSTR_RANGE(exec range=0xffffffc000083d9c:[0xffffffc000083dac] num_i(4) last_sz(4) (ISA=A64) E iBR b+link )
Idx:17293; ID:10; [0x95 0xf7 0x09 ];	I_ADDR_S_IS0 : Address, Short, IS0.; Addr=0xFFFFFFC0000813DC ~[0x13DC]
Idx:17297; ID:10; [0xdb ];	I_ATOM_F2 : Atom format 2.; EE
Idx:17297; ID:10; OCSD_GEN_TRC_ELEM_INSTR_RANGE(exec range=0xffffffc0000813dc:[0xffffffc0000813f0] num_i(5) last_sz(4) (ISA=A64) E BR  b+link )
Idx:17297; ID:10; OCSD_GEN_TRC_ELEM_INSTR_RANGE(exec range=0xffffffc00008f2e0:[0xffffffc00008f2e4] num_i(1) last_sz(4) (ISA=A64) E iBR A64:ret )
Idx:17298; ID:10; [0x95 0x7e ];	I_ADDR_S_IS0 : Address, Short, IS0.; Addr=0xFFFFFFC0000813F8 ~[0x1F8]
Idx:17300; ID:10; [0xe0 ];	I_ATOM_F6 : Atom format 6.; EEEN
Idx:17300; ID:10; OCSD_GEN_TRC_ELEM_INSTR_RANGE(exec range=0xffffffc0000813f8:[0xffffffc00008140c] num_i(5) last_sz(4) (ISA=A64) E BR  )
Idx:17300; ID:10; OCSD_GEN_TRC_ELEM_INSTR_RANGE(exec range=0xffffffc00008141c:[0xffffffc000081434] num_i(6) last_sz(4) (ISA=A64) E BR   <cond>)
Idx:17300; ID:10; OCSD_GEN_TRC_ELEM_INSTR_RANGE(exec range=0xffffffc00008140c:[0xffffffc000081414] num_i(2) last_sz(4) (ISA=A64) E BR  b+link )
Idx:17300; ID:10; OCSD_GEN_TRC_ELEM_ADDR_NACC( 0xffffffc000117cf0 )

~~~~~~~~~~~~~~~~

The `c_api_pkt_print_test` program.
-----------------------------------

Program tests the C-API infrastructure, including as an option the external decoder support. 

Limited to decoding trace from a single CoreSight ID. Uses the same "snapshots" as the C++ test program, but using hardcoded path values.

__Command Line Options__

By default the program will run the single CoreSight ID of 0x10 in packet processing output mode using the ETMv4 decoder on the Juno snapshot.

- `-id <n>`          : Change the ID used for the test.
- `-etmv3`           : Test the ETMv3 decoder - uses the TC2 snapshot.
- `-ptm`             : Test the PTM decoder - uses the TC2 snapshot.
- `-stm`             : Test the STM decoder - uses juno STM only snapshot.
- `-extern`          : Use the 'echo_test' external decoder to test the custom decoder API.
- `-decode`          : Output trace protocol packets and full decode generic packets.
- `-decode_only`     : Output full decode generic packets only.

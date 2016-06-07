OpenCSD - An open source CoreSight(tm) Trace Decode library        {#mainpage}
===========================================================

This library provides an API suitable for the decode of ARM(r) CoreSight(tm) trace streams.

The library will decode formatted trace in three stages:-
1. *Frame Deformatting* : Removal CoreSight frame formatting from individual trace streams.
2. *Packet Processing*  : Separate individual trace streams into discrete packets.
3. *Packet Decode*      : Convert the packets into fully decoded trace describing the program flow on a core.

The library is implemented in C++ with an optional "C" API.


CoreSight Trace Component Support.
----------------------------------

_Current Version 0.003_

### Current support:

- ETMv4 instruction trace - packet processing and packet decode.
- PTM instruction trace - packet processing and packet decode.
- ETMv3 instruction trace - packet processing and packet decode.
- ETMv3 data trace - packet processing.
- STM software trace - packet processing.

### Support to be added:

- ITM software trace - packet processing.
- ETMv3 data trace - packet decode.
- ETMv4 data trace - packet processing and decode.

Note on the Git Repository.
---------------------------

At present, the git repository for OpenCSD contains both branches for the OpenCSD library itself, and branches that 
have the perf updates that are not yet upstream in the main linux tree for using perf to record and decode trace.

These perf branches are snapshots of the kernel tree and are thus quite large. 
It is advised if only the OpenCSD library is required, clone only selected branches.
Otherwise, downloading may take some time.

e.g.

    git clone -b opencsd-0v003 --single-branch https://github.com/Linaro/OpenCSD 


Documentation
-------------

API Documentation is provided inline in the source header files, which use the __doxygen__ standard mark-up.
Run `doxygen` on the `./doxygen_config.dox` file located in the `./docs` directory..

    doxygen ./doxygen_config.dox

This will produce the documentation in the `./docs/html` directory. The doxygen configuration also includes
the `*.md` files as part of the documentation.


Building the Library
--------------------

See [build_libs.md](@ref build_lib) in the `./docs` directory for build details.


How the Library is used in Linux `perf`
---------------------------------------
The library and additional infrastructure for programming CoreSight components has been integrated 
with the standard linux perfomance analysis tool `perf`.


See [HOWTO.md](@ref howto_perf) for details.


Version and Modification Information
====================================

- _Version 0.001_:  Library development - tested with `perf` tools integration - BKK16, 8th March 2016
- _Version 0.002_:  Library development - added in PTM decoder support. Restructure header dir, replaced ARM rctdl prefix with opencsd/ocsd.
- _Version 0.003_:  Library development - added in ETMv3 instruction decoder support.

Licence Information
===================

This library is licensed under the [BSD three clause licence.](http://directory.fsf.org/wiki/License:BSD_3Clause)

A copy of this license is in the `LICENCE` file included with the source code.

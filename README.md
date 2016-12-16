OpenCSD - An open source CoreSight(tm) Trace Decode library        {#mainpage}
===========================================================

This library provides an API suitable for the decode of ARM(r) CoreSight(tm) trace streams.

The library will decode formatted trace in three stages:-
1. *Frame Deformatting* : Removal CoreSight frame formatting from individual trace streams.
2. *Packet Processing*  : Separate individual trace streams into discrete packets.
3. *Packet Decode*      : Convert the packets into fully decoded trace describing the program flow on a core.

The library is implemented in C++ with an optional "C" API.

Library Versioning
------------------

From version 0.4, library versioning will use a semantic versioning format
(per http://semver.org) of the form _Major.minor.patch_ (M.m.p).

Internal library version calls, documentation and git repository will use this format moving forwards.
Where a patch version is not quoted, or quoted as .x then comments will apply to the entire release.

Releases will be at M.m.0, with patch version incremented for bugfixes or documentation updates.

Releases will appear on the master branch in the git repository with an appropriate version tag.

CoreSight Trace Component Support.
----------------------------------

_Current Version 0.5.1_

### Current support:

- ETMv4 instruction trace - packet processing and packet decode.
- PTM instruction trace - packet processing and packet decode.
- ETMv3 instruction trace - packet processing and packet decode.
- ETMv3 data trace - packet processing.
- STM software trace - packet processing and packet decode.

- External Decoders - support for addition of external / custom decoders into the library.

### Support to be added:

- ITM software trace - packet processing and decode.
- ETMv3 data trace - packet decode.
- ETMv4 data trace - packet processing and decode.

Note: for ITM and STM, packet decode is combining Master+Channel+Marker+Payload packets into a single generic
output packet.


Note on the Git Repository.
---------------------------

At present, the git repository for OpenCSD contains both branches/tags for the OpenCSD library itself, and branches that 
have the perf updates that are not yet upstream in the main linux tree for using perf to record and decode trace.

These perf branches are snapshots of the kernel tree and are thus quite large. 
It is advised if only the OpenCSD library is required, clone only selected branches.
Otherwise, downloading may take some time.

e.g.

    git clone -b opencsd-0v003 --single-branch https://github.com/Linaro/OpenCSD 

(From version 0.4, releases appear as versioned tags on the master branch.)


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
- _Version 0.4_  :  Library development - updated decode tree and C-API for generic decoder handling. Switch to semantic versioning.
- _Version 0.4.1_:  Minor Update & Bugfixes - fix to PTM decoder, ID checking on test program, adds NULL_TS support in STM packet processor.
- _Version 0.4.2_:  Minor Update - Update to documentation for perf usage in 4.8 kernel branch.
- _Version 0.5.0_:  Library Development - external decoder support. STM full decode.
- _Version 0.5.1_:  Minor Update & Bugfixes - Update HOWTO for kernel 4.9. Build fixes for parallel builds

Licence Information
===================

This library is licensed under the [BSD three clause licence.](http://directory.fsf.org/wiki/License:BSD_3Clause)

A copy of this license is in the `LICENCE` file included with the source code.

Contact
=======

Using the github site: https://github.com/Linaro/OpenCSD

Mailing list: coresight@lists.linaro.org

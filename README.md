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

_Current Version 0.001_

### Current support:

- ETMv4 instruction trace - packet processing and packet decode.
- ETMv3 instruction and data trace - packet processing.
- STM software trace - packet processing.

### Support to be added:

- ETMv3 instruction trace - packet decode.
- PTM instruction trace - packet processing and decode.
- ITM software trace - packet processing.
- ETMv3 data trace - packet decode.
- ETMv4 data trace - packet processing and decode.


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


Version and Modification Information
====================================

Version 0.001
-------------

Library initial development phase.


Licence Information
===================

This library is licensed under the [BSD three clause licence.](http://directory.fsf.org/wiki/License:BSD_3Clause)

A copy of this license is in the `LICENCE` file included with the source code.

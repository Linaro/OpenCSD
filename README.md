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


Version and Modification Information
====================================

Version 0.001
-------------

Library initial development phase.


Licence Information
===================

This library is licensed under the [BSD three clause licence.](http://directory.fsf.org/wiki/License:BSD_3Clause)

A copy of this license is in the `LICENCE` file included with the source code.

# Source Address after compressed returns

Reduced AArch64 ETE regression for a pending return-stack pop followed by a
Source Address element. Return-stack compression is enabled (TRCCONFIGR=0x1000).

Sessions:

1. Simple compressed return followed by Source Address (db b4 02 f7).
2. Simple atom-only control (db f7 f7).
3. Simple explicit-target control (db 9d 01 08 00 00 00 00 00 00 b4 02 f7).
4. Nested compressed returns followed by Source Address (ff b4 0c f7).
5. Nested atom-only control (ff f7 f7).

The simple image is loaded at 0x1000. Sessions 1-3 must include the caller
range [0x1004, 0x100c), num_i(2), and finish at the branch at 0x1018.
The nested image is loaded at 0x2000. Sessions 4-5 must include
[0x2028, 0x2034), num_i(3), then [0x2004, 0x2008), num_i(1).
All code words and trace suffixes derive from source_return_test.py; nested
addresses are relocated by 0x1000 so both images share one core memory map.

The default run decodes session 1. Use -multi_session to decode all five.
The ETE suite also exercises the default session with -src_addr_n.

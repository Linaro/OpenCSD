# Source Address exception-return notification

AArch64 ETE trace with return-stack compression disabled. The image is:

```
1000  b5000061  CBNZ X1, 0x100c  // X1 is zero: falls through
1004  d69f03e0  ERET             // ELR_EL1 = 0x1008, same context
1008  14000000  B 0x1008
100c  14000000  B 0x100c
1010  b5000061  CBNZ X1, 0x101c  // ordinary RET control
1014  d65f03c0  RET              // X30 = 0x1018
1018  14000000  B 0x1018
```

Sessions 1-3 identify the ERET through short, 32-bit, and 64-bit Source Address
packets respectively. Session 4 uses N,E atoms as the equivalent control.
Session 5 identifies the ordinary RET through a short Source Address packet.
Each session supplies an explicit target address followed by an E atom.

Sessions 1-4 must emit exactly one OCSD_GEN_TRC_ELEM_EXCEPTION_RET after the
range ending at ERET and before the range starting at 0x1008. Session 5 must
emit no such notification. Both normal and -src_addr_n output must satisfy
these expectations. The default run uses session 1; -multi_session uses all five.

The standalone regression script checks counts and ordering:

```
python decoder/tests/standalone_regression_scripts/source_eret_test.py PATH_TO_TRC_PKT_LISTER
```

Its results are written to standalone_regression_scripts/results/source_eret_test.ppl.

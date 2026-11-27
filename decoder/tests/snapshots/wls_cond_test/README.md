# wls_cond_test

Regression for OPENCSD-BUGS.md issue 4.
Each opcode is exercised with an N atom and an E atom, followed by an E
for the destination self-branch. Independent cases begin with A-Sync,
Trace Info and Address with Context. Run with -direct_br_cond.

- 0x1000: 0xf044c003
- 0x1100: 0xf004c003

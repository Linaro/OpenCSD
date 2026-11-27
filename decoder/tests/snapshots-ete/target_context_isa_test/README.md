# Target address IS followed by context (issue 6)

All five sessions start in A64 Secure EL1 at ERET (0x1000), with explicit
return targets and return-stack compression disabled.

1. IS1 target 0x1012, then AArch32 context: two-byte T32 B .
2. AArch32 context, then IS1 target 0x1012: same result.
3. IS0 target 0x1020, then AArch32 context: four-byte A32 B .
4. IS0 target 0x1040, then AArch64 context: four-byte A64 B .
5. IS1 target 0x1030, then AArch32 context: T32 BLX to A32 at 0x1020.
   A second AArch32 context before the next atom must preserve A32.

Session 1 is the default. Use -multi_session for all five. The standalone
script target_context_isa_test.py checks the instruction ranges and ISAs;
its output is results/target_context_isa_test.ppl beside the script.

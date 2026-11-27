"""Regression for wls_cond_test; pass the trc_pkt_lister executable as argv[1]."""
from pathlib import Path
import subprocess
import sys
script = Path(__file__).resolve()
results = script.parent / 'results'
results.mkdir(exist_ok=True)
log = results / script.with_suffix('.ppl').name
log.write_text('')
exe = Path(sys.argv[1]).resolve()
snapshot = script.parents[1] / 'snapshots' / 'wls_cond_test'
for strict in (False, True):
    start = len(log.read_text())
    subprocess.run([str(exe), '-ss_dir', str(snapshot), '-decode', '-no_time_print',
                    *(['-direct_br_cond'] if strict else []), '-logfilename', str(log)],
                   check=True, stdout=subprocess.DEVNULL)
    text = log.read_text()[start:]
    assert 'bad-program-image' not in text and 'BAD_DECODE_IMAGE' not in text, text
    ranges = [line for line in text.splitlines() if 'INSTR_RANGE' in line]
    assert len(ranges) == 8, ranges
    for case in range(2):
        addr = 0x1000 + case * 0x100
        for outcome in range(2):
            branch, target = ranges[case*4+outcome*2:case*4+outcome*2+2]
            assert f'range=0x{addr:x}:' in branch and '<cond>' in branch, branch
            assert ('N BR' if outcome == 0 else 'E BR') in branch, branch
            expected = addr+4 if outcome == 0 else addr+8
            assert f'range=0x{expected:x}:' in target, target
    print(f'wls_cond_test passed (direct_br_cond={strict})')
print(f'Results: {log}')

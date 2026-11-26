"""Check ETE Source Address exception-return events; pass trc_pkt_lister as argv[1]."""
from pathlib import Path
import subprocess
import sys

script = Path(__file__).resolve()
snapshot = script.parents[1] / 'snapshots-ete' / 'src_addr_eret_test'
results = script.parent / 'results'
results.mkdir(exist_ok=True)
log = results / script.with_suffix('.ppl').name
log.write_text('')
exe = Path(sys.argv[1]).resolve()
for split in (False, True):
    start = len(log.read_text())
    subprocess.run([str(exe), '-ss_dir', str(snapshot), '-decode',
                    '-multi_session', '-no_time_print',
                    *(['-src_addr_n'] if split else []),
                    '-logfilename', str(log)], check=True, stdout=subprocess.DEVNULL)
    sessions = log.read_text()[start:].split('####### Multi Session decode:')[1:]
    assert len(sessions) == 5, f'Expected five sessions, got {len(sessions)}'
    for number, session in enumerate(sessions, 1):
        lines = [line for line in session.splitlines() if 'OCSD_GEN_TRC_ELEM_' in line]
        events = [i for i, line in enumerate(lines) if 'OCSD_GEN_TRC_ELEM_EXCEPTION_RET' in line]
        assert len(events) == (1 if number < 5 else 0), (number, lines)
        if number < 5:
            i = events[0]
            assert i > 0 and 'A64:eret' in lines[i-1], lines
            assert 'E iBR' in lines[i-1], lines
            assert i+1 < len(lines) and 'range=0x1008:' in lines[i+1], lines
        else:
            assert any('A64:ret' in line for line in lines), lines
        assert not any('NO_SYNC' in line for line in lines[1:]), lines
    print(f'Five sessions passed (src_addr_n={split})')
print(f'Results: {log}')

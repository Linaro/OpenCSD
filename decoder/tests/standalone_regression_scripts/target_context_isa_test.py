"""Check issue 6 target/context ISA resolution; argv[1] is trc_pkt_lister."""
from pathlib import Path
import subprocess
import sys
script = Path(__file__).resolve()
results = script.parent / 'results'
results.mkdir(exist_ok=True)
log = results / script.with_suffix('.ppl').name
log.write_text('')
snapshot = script.parents[1] / 'snapshots-ete' / 'target_context_isa_test'
subprocess.run([str(Path(sys.argv[1]).resolve()), '-ss_dir', str(snapshot),
                '-decode', '-multi_session', '-no_time_print', '-logfilename', str(log)],
               check=True, stdout=subprocess.DEVNULL)
sessions = log.read_text().split('####### Multi Session decode:')[1:]
assert len(sessions) == 5, len(sessions)
expected = [(0x1012, 2, 'T32'), (0x1012, 2, 'T32'),
            (0x1020, 4, 'A32'), (0x1040, 4, 'A64'), (0x1020, 4, 'A32')]
for session, (addr, size, isa) in zip(sessions, expected):
    ranges = [line for line in session.splitlines() if 'INSTR_RANGE' in line]
    assert len(ranges) == (3 if addr == 0x1020 and '0x1030:' in session else 2), ranges
    last = ranges[-1]
    assert f'range=0x{addr:x}:[0x{addr+size:x}]' in last, last
    assert f'last_sz({size})' in last and f'(ISA={isa})' in last, last
    assert 'bad-program-image' not in session, session
print(f'Five target/context ISA sessions passed. Results: {log}')

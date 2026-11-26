from pathlib import Path
import shutil, struct, subprocess, tempfile, sys
script = Path(__file__).resolve()
root = script.parents[3]
results = script.parent / "results"
results.mkdir(exist_ok=True)
log = results / script.with_suffix(".ppl").name
log.write_text("")
exe = Path(sys.argv[1]).resolve()
prefix = bytes(11) + bytes.fromhex('80 01 00 04 85 00 08 00 00 00 00 00 00 31')
with tempfile.TemporaryDirectory(prefix='opencsd-source-return-') as tmp:
    out = Path(tmp)
    for name in ('snapshot.ini', 'trace.ini'):
        shutil.copyfile(root/'decoder/tests/snapshots-ete/src_addr'/name, out/name)
    (out/'ETE_0_s1.ini').write_text('[device]\nname=ETE_0_s1\nclass=trace_source\ntype=ETE\n[regs]\nTRCIDR0=0x2801cea1\nTRCIDR1=0x4100fff0\nTRCIDR2=0xd0001088\nTRCDEVARCH=0x47715a13\nTRCCONFIGR=0x1000\nTRCTRACEIDR=1\nTRCIDR8=0\n')
    words = (0x94000004,0xd503201f,0x14000004,0xd503201f,0xd65f03c0,0xd503201f,0x14000000)
    (out/'code.bin').write_bytes(struct.pack('<7I',*words))
    (out/'cpu_0.ini').write_text('[device]\nname=cpu_0\nclass=core\ntype=ARM-AA64\n[dump1]\nfile=code.bin\naddress=0x1000\nlength=0x1c\n')
    for suffix in ('db b4 02 f7','db f7 f7','db 9d 01 08 00 00 00 00 00 00 b4 02 f7'):
        (out/'session1.bin').write_bytes(prefix+bytes.fromhex(suffix))
        start = len(log.read_text())
        subprocess.run([str(exe),'-ss_dir',str(out),'-decode','-logfilename',str(log)],check=True,stdout=subprocess.DEVNULL)
        ranges=[x for x in log.read_text()[start:].splitlines() if 'INSTR_RANGE' in x]
        print(suffix, '\n'+'\n'.join(ranges))
        assert any('1004' in x and '100c' in x.lower() and 'num_i(2)' in x for x in ranges), ranges
    nested = [0xd503201f]*17
    for addr, op in {0x1000:0x94000008,0x1004:0x14000000,0x1020:0xaa1e03e4,0x1024:0x94000007,0x102c:0xaa0403fe,0x1030:0xd65f03c0,0x1040:0xd65f03c0}.items():
        nested[(addr-0x1000)//4]=op
    (out/'code.bin').write_bytes(struct.pack('<17I',*nested))
    cpu=out/'cpu_0.ini'
    cpu.write_text(cpu.read_text().replace('length=0x1c','length=0x44'))
    for suffix in ('ff b4 0c f7','ff f7 f7'):
        (out/'session1.bin').write_bytes(prefix+bytes.fromhex(suffix))
        start = len(log.read_text())
        subprocess.run([str(exe),'-ss_dir',str(out),'-decode','-logfilename',str(log)],check=True,stdout=subprocess.DEVNULL)
        ranges=[x for x in log.read_text()[start:].splitlines() if 'INSTR_RANGE' in x]
        assert 'range=0x1004:[0x1008]' in ranges[-1], ranges
        print('Nested return passed:', suffix)
print('Source Address compressed-return checks passed')

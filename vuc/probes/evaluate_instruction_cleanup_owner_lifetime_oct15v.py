#!/usr/bin/env python3
"""Inspect genuine 2004-era Instruction arena teardown source lifetimes.

Never patches instructions, register choices, ELF offsets, relocation records,
or artificial NOPs. All candidates retain real GNU C++ global destruction.
"""
from pathlib import Path
import subprocess,importlib.util,json
R=Path(__file__).resolve().parents[1];O=Path('/tmp/vuc-instruction-cleanup-oct15v');O.mkdir(exist_ok=True)
base=(R/'probes/instruction_arena_cleanup.cpp').read_text()
start=base.index('        while (Instruction::instrData) {')
end=base.index('\n    }\n};',start)
prefix=base[:start]; suffix=base[end:]
bodies={
 'baseline':'''while (Instruction::instrData) { char *current = Instruction::instrData; char *next=*(char**)(current+0x2004); ::operator delete(current); Instruction::instrData=next; }''',
 'for-test-global':'''for (; Instruction::instrData; ) { char *current=Instruction::instrData; char *next=*(char**)(current+0x2004); ::operator delete(current); Instruction::instrData=next; }''',
 'for-increment':'''for (char *p=Instruction::instrData; p; p=Instruction::instrData) { char *next=*(char**)(p+0x2004); ::operator delete(p); Instruction::instrData=next; }''',
 'for-next-step':'''for (char *p=Instruction::instrData; p; ) { char *next=*(char**)(p+0x2004); ::operator delete(p); Instruction::instrData=p=next; }''',
 'while-local-p':'''char *p=Instruction::instrData; while (p) { char *next=*(char**)(p+0x2004); ::operator delete(p); Instruction::instrData=next; p=next; }''',
 'while-next-from-global':'''while (Instruction::instrData) { char *p=Instruction::instrData; char *next=*(char**)(Instruction::instrData+0x2004); ::operator delete(p); Instruction::instrData=next; }''',
 'while-first-delete':'''while (Instruction::instrData) { char *p=Instruction::instrData; char *next=*(char**)(p+0x2004); delete [] p; Instruction::instrData=next; }''',
 'do-while':'''if (Instruction::instrData) do { char *p=Instruction::instrData; char *next=*(char**)(p+0x2004); ::operator delete(p); Instruction::instrData=next; } while(Instruction::instrData);''',
 'do-while-p':'''char *p=Instruction::instrData; if(p) do { char *next=*(char**)(p+0x2004); ::operator delete(p); Instruction::instrData=p=next; } while(p);''',
 'while-post-test':'''char *p=Instruction::instrData; while(p) { char *next=*(char**)(p+0x2004); ::operator delete(p); p=next; Instruction::instrData=p; }''',
 'reference-global':'''char *&head=Instruction::instrData; while(head) { char *p=head; char *next=*(char**)(p+0x2004); ::operator delete(p); head=next; }''',
 'reordered-write':'''while(Instruction::instrData) { char *p=Instruction::instrData; char *next=*(char**)(p+0x2004); Instruction::instrData=next; ::operator delete(p); }''',
 'pointer-slot':'''while(Instruction::instrData) { char *p=Instruction::instrData; char **next=(char**)(p+0x2004); char *value=*next; ::operator delete(p); Instruction::instrData=value; }''',
 'next-byte':'''while(Instruction::instrData) { char *p=Instruction::instrData; char *next=*((char**)(p+8196)); ::operator delete(p); Instruction::instrData=next; }''',
 'explicit-null':'''while(Instruction::instrData!=0) { char *p=Instruction::instrData; char *next=*((char**)(p+8196)); ::operator delete(p); Instruction::instrData=next; }''',
 'unroll-control':'''while(Instruction::instrData) { char *p=Instruction::instrData; char *next=*((char**)(p+8196)); ::operator delete(p); Instruction::instrData=next; if(!next) break; }''',
}
spec=importlib.util.spec_from_file_location('match',R/'scripts/match_probe.py');mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod)
for name,body in bodies.items():
 path=O/(name+'.cpp');path.write_text(prefix+body+suffix)
 for cc in ('jan25','dec06'):
  obj=O/(name+'-'+cc+'.o');cmd=['/tmp/vuc-binutils215/g++-'+cc+'-as215','-O2','-frename-registers','-fuse-cxa-atexit','-c',str(path),'-o',str(obj)];p=subprocess.run(cmd,text=True,capture_output=True)
  if p.returncode:print('FAIL',name,cc,p.stderr[-130:],flush=True);continue
  try:
   score=mod.run(obj,R/'config/instruction_arena_cleanup_probe.json');print('RESULT',name,cc,score['exact_bytes'],score['length'],score['different_offsets'][:12],flush=True)
   if score['exact']:print('BYTE_EXACT',name,cc,flush=True);(O/'EXACT.txt').write_text(name+' '+cc+'\n')
  except Exception as e:print('REJECT',name,cc,str(e)[:150],flush=True)

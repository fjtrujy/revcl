#!/usr/bin/env python3
"""Recompile authentic Instruction methods in plausible historical C++ source orders."""
import importlib.util
import itertools
import subprocess
from pathlib import Path
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/instruction-order-oct13')
T.mkdir(parents=True, exist_ok=True)
spec=importlib.util.spec_from_file_location('matcher',R/'vuc/scripts/match_probe.py')
matcher=importlib.util.module_from_spec(spec);spec.loader.exec_module(matcher)
src=(R/'vuc/probes/instruction_full_source_unit_oct13.cpp').read_text()
header=src[:src.index('char *Instruction::instrData;')]+'char *Instruction::instrData;\n'
header=header.replace('#include "../src/MLGameString.h"', '#include "'+str(R/'vuc/src/MLGameString.h')+'"')
chunks={
 'text':src[src.index('const char *Instruction::ToString()'):src.index('void *Instruction::operator new(')],
 'allocator':src[src.index('void *Instruction::operator new('):src.index('struct InstructionArenaCleanup')],
 'lifecycle':src[src.index('struct InstructionArenaCleanup'):],
}
flags={
 'base':['-O2','-frename-registers','-fuse-cxa-atexit'],
 'j1':['-O2','-frename-registers','-fuse-cxa-atexit','-falign-jumps=1'],
 'sections':['-O2','-frename-registers','-fuse-cxa-atexit','-ffunction-sections'],
 'plain':['-O2','-fuse-cxa-atexit'],
 'r-no-peep':['-O2','-frename-registers','-fuse-cxa-atexit','-fno-peephole2'],
 'r-no-sched':['-O2','-frename-registers','-fuse-cxa-atexit','-fno-schedule-insns2'],
}
results=[]
for seq in itertools.permutations(chunks):
 label='-'.join(seq)
 source=T/(label+'.cpp');source.write_text(header+'\n'.join(chunks[c] for c in seq))
 for tag,fl in flags.items():
  obj=T/(label+'-'+tag+'.o')
  cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*fl,'-c',str(source),'-o',str(obj)],text=True,capture_output=True)
  if cp.returncode:
   print(label,tag,'compile-error',cp.stderr[-150:],flush=True);continue
  try:
   r=matcher.run(obj,R/'vuc/config/instruction_arena_cleanup_unit_oct14_probe.json')
   results.append((r['exact_bytes'],r['length'],label,tag,r['exact']))
   if r['exact']:
    print('FULL EXACT',label,tag,flush=True)
    raise SystemExit(0)
  except ValueError as e:
   results.append((0,0,label,tag,str(e)[:100]))
 print('checked',label,flush=True)
for row in sorted(results,reverse=True)[:26]:print(*row)

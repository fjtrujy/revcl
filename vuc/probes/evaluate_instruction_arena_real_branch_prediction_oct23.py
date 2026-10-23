#!/usr/bin/env python3
"""Try authentic InstructionArenaCleanup loop-likelihood and early-exit idioms.

One remaining dead epilogue scratch register in __tcf_1: original pop ecx
versus recovered GCC 3.3.3 pop ebx/eax. Only generated C++03 destructor
source is varied, keeping actual arena deletion semantics and proven symbols.
"""
from pathlib import Path
import json,subprocess,importlib.util,re
R=Path(__file__).resolve().parents[1]
T=Path('/tmp/vuc-tcf-likelihood-20261023');T.mkdir(exist_ok=True)
spec=importlib.util.spec_from_file_location('m',R/'scripts/match_probe.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
src=(R/'probes/instruction_list_arena_unit_oct14.cpp').read_text()
old='''        while (Instruction::instrData) {
            char *current = Instruction::instrData;
            char *next = *(char **)(current + 0x2004);
            ::operator delete(current);
            Instruction::instrData = next;
        }'''
assert old in src
variants={}
for likely in (0,1):
 variants[f'while-expect-{likely}']=old.replace('while (Instruction::instrData)',f'while (__builtin_expect(Instruction::instrData != 0, {likely}))')
 variants[f'while-not-expect-{likely}']=old.replace('while (Instruction::instrData)',f'while (!__builtin_expect(Instruction::instrData == 0, {likely}))')
 variants[f'break-expect-{likely}']='''        while (true) {
            if (__builtin_expect(Instruction::instrData == 0, '''+str(likely)+''')) break;
            char *current = Instruction::instrData;
            char *next = *(char **)(current + 0x2004);
            ::operator delete(current);
            Instruction::instrData = next;
        }'''
 variants[f'for-expect-{likely}']=old.replace('while (Instruction::instrData)',f'for (; __builtin_expect(Instruction::instrData != 0, {likely}); )')
rows=[]
for name,body in variants.items():
 cpp=T/(name+'.cpp');cpp.write_text(src.replace(old,body).replace('#include "../src/MLGameString.h"','#include "'+str(R/'src/MLGameString.h')+'"'))
 row={'case':name}
 for snap in ('jan25','dec06'):
  obj=T/(name+'-'+snap+'.o')
  cp=subprocess.run(['/tmp/vuc-binutils215/g++-'+snap+'-as215','-O2','-frename-registers','-fuse-cxa-atexit','-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
  if cp.returncode:row[snap]={'error':cp.stderr[-350:]};continue
  try:
   result=m.run(obj,R/'config/instruction_arena_cleanup_unit_oct14_probe.json')
   row[snap]={'exact_bytes':result['exact_bytes'],'length':result['length'],'diff':result['different_offsets']}
   if result['exact']:print('EXACT',name,snap,flush=True)
  except ValueError as e:row[snap]={'error':str(e)}
 rows.append(row)
 print(name,row['jan25'],flush=True)
(T/'results.json').write_text(json.dumps(rows,indent=2)+'\n')

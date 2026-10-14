#!/usr/bin/env python3
"""Genuine GCC 3.3.3 arena destructor loop branch-likelihood controls.

The original VCL setLocks use of paired neutral expect conditions suggested
checking VUC's actual pointer-loop guard and its cleanup-after-delete edge.
No forced register names, object code changes or synthetic predecessors.
"""
from pathlib import Path
import subprocess,importlib.util,itertools
R=Path(__file__).resolve().parents[2];T=Path('/tmp/vuc-binutils215/arena-loop-pred-oct14')
T.mkdir(parents=True,exist_ok=True)
s=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
base=(R/'vuc/probes/instruction_full_source_unit_oct13.cpp').read_text().replace(
 '"../src/MLGameString.h"','"'+str(R/'vuc/src/MLGameString.h')+'"')
stem='''        while (Instruction::instrData) {
            char *current = Instruction::instrData;
            char *next = *(char **)(current + 0x2004);
            ::operator delete(current);
            Instruction::instrData = next;
        }'''
assert stem in base
variants={
'base':stem,
'while-likely':stem.replace('while (Instruction::instrData)','while (__builtin_expect(Instruction::instrData != 0, 1))'),
'while-unlikely':stem.replace('while (Instruction::instrData)','while (__builtin_expect(Instruction::instrData != 0, 0))'),
'while-null-likely':stem.replace('while (Instruction::instrData)','while (!__builtin_expect(Instruction::instrData == 0, 0))'),
'while-null-unlikely':stem.replace('while (Instruction::instrData)','while (!__builtin_expect(Instruction::instrData == 0, 1))'),
'while-ref':stem.replace('while (Instruction::instrData)','while (char *ptr = Instruction::instrData)').replace('char *current = Instruction::instrData;','char *current = ptr;'),
'while-ref-likely':stem.replace('while (Instruction::instrData)','while (__builtin_expect(Instruction::instrData != 0, 1))').replace('char *current = Instruction::instrData;','char *current = Instruction::instrData;'),
'for-likely':stem.replace('while (Instruction::instrData)','for (; __builtin_expect(Instruction::instrData != 0, 1); )'),
'for-unlikely':stem.replace('while (Instruction::instrData)','for (; __builtin_expect(Instruction::instrData != 0, 0); )'),
'if-do-likely':stem.replace('while (Instruction::instrData) {','if (__builtin_expect(Instruction::instrData != 0, 1)) do {').replace('        }','        } while (Instruction::instrData);'),
'if-do-unlikely':stem.replace('while (Instruction::instrData) {','if (__builtin_expect(Instruction::instrData != 0, 0)) do {').replace('        }','        } while (Instruction::instrData);'),
'loop-local':stem.replace('while (Instruction::instrData)','while (Instruction::instrData)').replace(
 'Instruction::instrData = next;','Instruction::instrData = next;'),
'while-guard-likely':'''        for (;;) {
            if (__builtin_expect(Instruction::instrData == 0, 0)) break;
            char *current = Instruction::instrData;
            char *next = *(char **)(current + 0x2004);
            ::operator delete(current);
            Instruction::instrData = next;
        }''',
'while-guard-unlikely':'''        for (;;) {
            if (__builtin_expect(Instruction::instrData == 0, 1)) break;
            char *current = Instruction::instrData;
            char *next = *(char **)(current + 0x2004);
            ::operator delete(current);
            Instruction::instrData = next;
        }'''
}
opts={
'r':['-O2','-frename-registers','-fuse-cxa-atexit','-ffunction-sections'],
'p':['-O2','-fuse-cxa-atexit','-ffunction-sections'],
'rj1':['-O2','-frename-registers','-fuse-cxa-atexit','-ffunction-sections','-falign-jumps=1'],
'rj4':['-O2','-frename-registers','-fuse-cxa-atexit','-ffunction-sections','-falign-jumps=4'],
'cost0':['-O2','-frename-registers','-fuse-cxa-atexit','-ffunction-sections','-mbranch-cost=0'],
'no-regmove':['-O2','-frename-registers','-fuse-cxa-atexit','-ffunction-sections','-fno-regmove'],
}
rows=[]
for name,body in variants.items():
 cpp=T/(name+'.cpp');cpp.write_text(base.replace(stem,body))
 for f,flags in opts.items():
  obj=T/(name+'-'+f+'.o')
  cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*flags,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
  if cp.returncode:
   print('COMPILE_ERROR',name,f,cp.stderr[-100:],flush=True);continue
  try:
   mres=m.run(obj,R/'vuc/config/instruction_arena_cleanup_probe.json')
   rows.append((mres['exact_bytes'],mres['length'],name,f,mres['different_offsets'][:8]))
   if mres['exact']:
    print('NEW_EXACT',name,f,cpp,flush=True);raise SystemExit(0)
  except ValueError as e:rows.append((0,0,name,f,str(e)[:130]))
 print('DONE',name,flush=True)
print('TOP')
for row in sorted(rows,reverse=True)[:35]:print(row,flush=True)

#!/usr/bin/env python3
"""Test genuine arena destructor source equivalences and target compiler flags."""
from pathlib import Path
import importlib.util, subprocess
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/instruction-cleanup-equivalents-oct13');T.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
source=(R/'vuc/probes/instruction_full_source_unit_oct13.cpp').read_text().replace('"../src/MLGameString.h"','"'+str(R/'vuc/src/MLGameString.h')+'"')
old='''        while (Instruction::instrData) {
            char *current = Instruction::instrData;
            char *next = *(char **)(current + 0x2004);
            ::operator delete(current);
            Instruction::instrData = next;
        }'''
assert old in source
forms={
 'base':old,
 'for':'''        for (char *current = Instruction::instrData; current;
             current = Instruction::instrData) {
            char *next = *(char **)(current + 0x2004);
            ::operator delete(current);
            Instruction::instrData = next;
        }''',
 'do-if':'''        if (Instruction::instrData) do {
            char *current = Instruction::instrData;
            char *next = *(char **)(current + 0x2004);
            ::operator delete(current);
            Instruction::instrData = next;
        } while (Instruction::instrData);''',
 'for-data':'''        for (; Instruction::instrData; ) {
            char *current = Instruction::instrData;
            char *next = *(char **)(current + 0x2004);
            ::operator delete(current);
            Instruction::instrData = next;
        }''',
 'while-aliased':'''        while (Instruction::instrData) {
            char *current = Instruction::instrData;
            char **next = (char **)(current + 0x2004);
            char *following = *next;
            ::operator delete(current);
            Instruction::instrData = following;
        }''',
 'while-index':'''        while (Instruction::instrData) {
            char *current = Instruction::instrData;
            char *next = ((char **)current)[0x801];
            ::operator delete(current);
            Instruction::instrData = next;
        }''',
 'while-free':'''        while (Instruction::instrData) {
            char *current = Instruction::instrData;
            char *next = *(char **)(current + 0x2004);
            delete[] current;
            Instruction::instrData = next;
        }''',
 'while-std':'''        while (Instruction::instrData) {
            char *current = Instruction::instrData;
            char *next = *(char **)(current + 0x2004);
            operator delete(current);
            Instruction::instrData = next;
        }''',
 'while-early':'''        for (;;) {
            char *current = Instruction::instrData;
            if (!current) break;
            char *next = *(char **)(current + 0x2004);
            ::operator delete(current);
            Instruction::instrData = next;
        }''',
 'read-after':'''        while (Instruction::instrData) {
            char *current = Instruction::instrData;
            char *next = *(char **)(current + 0x2004);
            Instruction::instrData = next;
            ::operator delete(current);
        }''',
}
flgs={
 'ren':['-O2','-frename-registers','-fuse-cxa-atexit','-ffunction-sections'],
 'plain':['-O2','-fuse-cxa-atexit','-ffunction-sections'],
 'j1':['-O2','-frename-registers','-fuse-cxa-atexit','-ffunction-sections','-falign-jumps=1'],
 'noregmove':['-O2','-frename-registers','-fuse-cxa-atexit','-ffunction-sections','-fno-regmove'],
 'notsched':['-O2','-frename-registers','-fuse-cxa-atexit','-ffunction-sections','-fno-schedule-insns2'],
 'march386':['-O2','-frename-registers','-fuse-cxa-atexit','-ffunction-sections','-march=i386'],
 'march686':['-O2','-frename-registers','-fuse-cxa-atexit','-ffunction-sections','-march=i686'],
 'noreorder':['-O2','-frename-registers','-fuse-cxa-atexit','-ffunction-sections','-fno-reorder-blocks'],
}
rank=[]
for name,body in forms.items():
 p=T/(name+'.cpp');p.write_text(source.replace(old,body))
 for flagname,flags in flgs.items():
  obj=T/(name+'-'+flagname+'.o')
  cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*flags,'-c',str(p),'-o',str(obj)],capture_output=True,text=True)
  if cp.returncode:print('COMPILE_FAIL',name,flagname,cp.stderr[-140:],flush=True);continue
  try:
   r=m.run(obj,R/'vuc/config/instruction_arena_cleanup_probe.json')
   rank.append((r['exact_bytes'],name,flagname,r['length']))
   if r['exact']:
    print('BYTE EXACT',name,flagname,p,flush=True);raise SystemExit(0)
  except ValueError as e:rank.append((0,name,flagname,str(e)[:90]))
 print('tested',name,flush=True)
for x in sorted(rank,reverse=True)[:30]:print('BEST',x)

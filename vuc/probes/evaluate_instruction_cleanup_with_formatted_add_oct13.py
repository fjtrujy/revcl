#!/usr/bin/env python3
"""Test recovered genuine formatted instruction methods preceding static arena cleanup."""
import importlib.util,subprocess
from pathlib import Path
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/instruction-cleanup-add-context-oct13');T.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
base=(R/'vuc/probes/instruction_list_arena_unit_oct14.cpp').read_text()
other=(R/'vuc/src/InstructionList.cpp').read_text()
base=base.replace('"../src/MLGameString.h"','"'+str(R/'vuc/src/MLGameString.h')+'"')
base=base.replace('#include <new>','#include <new>\n#include <cstdio>\n#include <cstdarg>')
base=base.replace('    void Add(Instruction *instruction);','    void Add(Instruction *instruction);\n    void Add(const char *format, ...);\n    void AddB(const char *format, ...);')
add=other[other.index('void InstructionList::Add(const char *format'):other.index('void InstructionList::AddB(const char *format')]
addb=other[other.index('void InstructionList::AddB(const char *format'):]
spots={
 'before-adl':'void InstructionList::AddL(',
 'after-addl':'MLGameString InstructionList::GenLabel(',
 'after-label':'MLGameString InstructionList::GenTmp(',
 'after-gen':'void InstructionList::Print(',
 'before-instr':'char *Instruction::instrData;',
 'before-arena':'struct InstructionArenaCleanup {',
 'before-static':'static InstructionArenaCleanup cleanup;',
}
flags={'r':['-O2','-frename-registers','-fuse-cxa-atexit','-ffunction-sections'],
 'plain':['-O2','-fuse-cxa-atexit','-ffunction-sections'],
 'j1':['-O2','-frename-registers','-fuse-cxa-atexit','-ffunction-sections','-falign-jumps=1'],
 'no-sched':['-O2','-frename-registers','-fuse-cxa-atexit','-ffunction-sections','-fno-schedule-insns2']}
rank=[]
for name,needle in spots.items():
 for payload,prefix in ((add,'add'),(addb,'addb'),(add+addb,'both'),(addb+add,'reverse')):
  cpp=T/(name+'-'+prefix+'.cpp')
  cpp.write_text(base.replace(needle,payload+'\n'+needle,1))
  for tag,opts in flags.items():
   obj=T/(name+'-'+prefix+'-'+tag+'.o')
   cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
   if cp.returncode:print('COMPILE_FAILED',name,prefix,tag,cp.stderr[-140:],flush=True);continue
   try:
    score=m.run(obj,R/'vuc/config/instruction_arena_cleanup_probe.json')
    rank.append((score['exact_bytes'],name,prefix,tag,score['length']))
    if score['exact']:
     print('BYTE-EXACT',name,prefix,tag,cpp,flush=True);raise SystemExit(0)
   except ValueError as e:rank.append((0,name,prefix,tag,str(e)[:90]))
 print('tested',name,flush=True)
for x in sorted(rank,reverse=True)[:30]:print('BEST',x)

#!/usr/bin/env python3
"""VUC line-comment 162-byte branch-arm source controls informed by VCL.

Uses genuine InstructionList unit and GCC 3.3.3. Test nested guard and
branch cost on original-sized ELF code without padding or instruction asm.
"""
from pathlib import Path
import importlib.util,subprocess
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/linecomment-guard-rebase-oct14');T.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
src=(R/'vuc/probes/instruction_line_with_unit_oct13.cpp').read_text().replace(
 '#include "../src/MLGameString.h"','#include "'+str(R/'vuc/src/MLGameString.h')+'"')
begin=src.index('void InstructionList::AddLineComment(int line)')
prefix=src[:begin]
original=src[begin:]
needle='''    if (instructions_.size()) {
        Instruction *previous = instructions_[instructions_.size() - 1];
        if (previous->kind == 3 && previous->reserved == line)
            return;
    }'''
assert needle in original
variants={
'original':needle,
'guard-empty':'''    if (instructions_.empty()) { } else {
        Instruction *previous = instructions_[instructions_.size() - 1];
        if (previous->kind == 3 && previous->reserved == line)
            return;
    }''',
'guard-nonempty':'''    if (!instructions_.empty()) {
        Instruction *previous = instructions_[instructions_.size() - 1];
        if (previous->kind == 3 && previous->reserved == line)
            return;
    }''',
'guard-line-first':'''    if (instructions_.size()) {
        Instruction *previous = instructions_[instructions_.size() - 1];
        if (previous->reserved == line && previous->kind == 3)
            return;
    }''',
'guard-kind-nested':'''    if (instructions_.size()) {
        Instruction *previous = instructions_[instructions_.size() - 1];
        if (previous->kind == 3) {
            if (previous->reserved == line) return;
        }
    }''',
'guard-line-nested':'''    if (instructions_.size()) {
        Instruction *previous = instructions_[instructions_.size() - 1];
        if (previous->reserved == line) {
            if (previous->kind == 3) return;
        }
    }''',
'guard-early':'''    if (!instructions_.size()) goto add_comment;
    {
        Instruction *previous = instructions_[instructions_.size() - 1];
        if (previous->kind == 3 && previous->reserved == line)
            return;
    }
add_comment: ;''',
'guard-goto':'''    if (instructions_.size()) {
        Instruction *previous = instructions_[instructions_.size() - 1];
        if (previous->kind != 3 || previous->reserved != line) goto add_comment;
        return;
    }
add_comment: ;''',
'guard-continue':'''    if (instructions_.size()) {
        Instruction *previous = instructions_[instructions_.size() - 1];
        if (!(previous->kind != 3 || previous->reserved != line))
            return;
    }''',
'guard-expect':'''    if (__builtin_expect(instructions_.size()!=0,0)) {
        Instruction *previous = instructions_[instructions_.size() - 1];
        if (previous->kind == 3 && previous->reserved == line)
            return;
    }''',
'guard-expect-kind':'''    if (instructions_.size()) {
        Instruction *previous = instructions_[instructions_.size() - 1];
        if (__builtin_expect(previous->kind == 3,0) && previous->reserved == line)
            return;
    }''',
'guard-expect-true':'''    if (instructions_.size()) {
        Instruction *previous = instructions_[instructions_.size() - 1];
        if (__builtin_expect(previous->kind == 3,1) && previous->reserved == line)
            return;
    }''',
}
opts={'j4':['-falign-jumps=4'], 'j4cost0':['-falign-jumps=4','-mbranch-cost=0'],
'j4cost1':['-falign-jumps=4','-mbranch-cost=1'],
'j4cost2':['-falign-jumps=4','-mbranch-cost=2'],
'j1':['-falign-jumps=1'],'j8':['-falign-jumps=8'],
'j4nothread':['-falign-jumps=4','-fno-thread-jumps'],
'j4noguess':['-falign-jumps=4','-fno-guess-branch-probability']}
scores=[]
for name,body in variants.items():
 path=T/(name+'.cpp');path.write_text(prefix+original.replace(needle,body))
 for key,extra in opts.items():
  o=T/(name+'-'+key+'.o')
  flags=['-O2','-frename-registers']+extra
  c=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*flags,'-c',str(path),'-o',str(o)],capture_output=True,text=True)
  if c.returncode:
   print('ERROR',name,key,c.stderr[-140:],flush=True);continue
  try:
   r=m.run(o,R/'vuc/config/instruction_add_line_comment_context_oct13_probe.json')
   scores.append((r['exact_bytes'],r['length'],name,key,r['different_offsets'][:14]))
   if r['exact']:
    print('NEW_EXACT',name,key,path,flush=True);raise SystemExit(0)
  except ValueError as e:scores.append((0,-1,name,key,str(e)[:120]))
 print('CHECKED',name,flush=True)
print('TOP')
for x in sorted(scores,reverse=True)[:30]:print(x,flush=True)

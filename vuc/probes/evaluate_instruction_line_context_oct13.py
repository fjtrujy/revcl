#!/usr/bin/env python3
"""Recover original AddLineComment CFG padding from real instruction source-unit order."""
import importlib.util,subprocess
from pathlib import Path
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/instruction-line-context-oct13');T.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
full=(R/'vuc/probes/instruction_line_with_unit_oct13.cpp').read_text()
head=full[:full.index('void InstructionList::AddLineComment(int line)')]
head=head.replace('"../src/MLGameString.h"', '"'+str(R/'vuc/src/MLGameString.h')+'"')
original='''if (instructions_.size()) {
 Instruction *previous = instructions_[instructions_.size()-1];
 if (previous->kind == 3 && previous->reserved == line) return;
}'''
action='''Instruction *instruction = new Instruction(MLGameString("", false));
 instruction->kind = 3;
 instruction->reserved = line;
 Add(instruction);'''
conds={
 'base':original,
 'nested':'''if (instructions_.size()) {
 Instruction *previous = instructions_[instructions_.size()-1];
 if (previous->kind == 3) {
  if (previous->reserved == line) return;
 }
}''',
 'nested-reverse':'''if (instructions_.size()) {
 Instruction *previous = instructions_[instructions_.size()-1];
 if (previous->reserved == line) {
  if (previous->kind == 3) return;
 }
}''',
 'boolean-not':'''if (instructions_.size()) {
 Instruction *previous = instructions_[instructions_.size()-1];
 if (!(previous->kind != 3 || previous->reserved != line)) return;
}''',
 'boolean-reverse':'''if (instructions_.size()) {
 Instruction *previous = instructions_[instructions_.size()-1];
 if (previous->reserved == line && previous->kind == 3) return;
}''',
 'positive-count':original.replace('instructions_.size())','instructions_.size()>0)'),
 'size-local':'''unsigned n=instructions_.size();
 if (n) {
 Instruction *previous = instructions_[n-1];
 if (previous->kind==3 && previous->reserved==line) return;
}''',
 'size-int':'''int n=(int)instructions_.size();
 if (n>0) {
 Instruction *previous = instructions_[n-1];
 if (previous->kind==3 && previous->reserved==line) return;
}''',
 'direct':'''if (instructions_.size() &&
 instructions_[instructions_.size()-1]->kind == 3 &&
 instructions_[instructions_.size()-1]->reserved == line) return;''',
 'end-index':'''if (instructions_.size()) {
 Instruction *previous = *(instructions_.end()-1);
 if (previous->kind==3 && previous->reserved==line) return;
}''',
 'goto':'''if (instructions_.size()) {
 Instruction *previous=instructions_[instructions_.size()-1];
 if (previous->kind==3 && previous->reserved==line) goto done;
}''',
 'last-local':'''Instruction *previous=0;
 if(instructions_.size()) previous=instructions_[instructions_.size()-1];
 if(previous && previous->kind==3 && previous->reserved==line) return;''',
 'if-flag':'''bool duplicate=false;
 if(instructions_.size()) {
 Instruction *previous=instructions_[instructions_.size()-1];
 duplicate=previous->kind==3 && previous->reserved==line;
 }
 if(duplicate) return;''',
}
opts={'j4':['-O2','-frename-registers','-falign-jumps=4'],
 'j4-l1':['-O2','-frename-registers','-falign-jumps=4','-falign-labels=1'],
 'j4-l4':['-O2','-frename-registers','-falign-jumps=4','-falign-labels=4'],
 'j4-loop1':['-O2','-frename-registers','-falign-jumps=4','-falign-loops=1'],
 'j4-noorder':['-O2','-frename-registers','-falign-jumps=4','-fno-reorder-blocks'],
 'plain-j4':['-O2','-falign-jumps=4']}
rank=[]
for name,cond in conds.items():
 suffix='done: ;' if name=='goto' else ''
 src=T/(name+'.cpp');src.write_text(head+'\nvoid InstructionList::AddLineComment(int line) {\n'+cond+'\n'+action+'\n'+suffix+'\n}\n')
 for tag,flags in opts.items():
  obj=T/(name+'-'+tag+'.o')
  cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*flags,'-c',str(src),'-o',str(obj)],capture_output=True,text=True)
  if cp.returncode:print('COMPILE_FAIL',name,tag,cp.stderr[-160:],flush=True);continue
  try:
   r=m.run(obj,R/'vuc/config/instruction_add_line_comment_context_oct13_probe.json')
   rank.append((r['exact_bytes'],name,tag,r['length']))
   if r['exact']:
    print('EXACT',name,tag,src,flush=True);raise SystemExit(0)
  except ValueError as e:
   rank.append((0,name,tag,str(e)[:90]))
 print('tested',name,flush=True)
for x in sorted(rank,reverse=True)[:27]:print('BEST',x)

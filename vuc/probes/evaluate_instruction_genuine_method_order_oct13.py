#!/usr/bin/env python3
"""Authentic Instruction method ordering and generated cleanup scratch search.

Only existing InstructionList/AddL/GenLabel/GenTmp/Print, Instruction
ToString/operator new and genuine arena static lifetime definitions can
precede __tcf_1. No fabricated helper functions, instructions or code bytes.
Verify the complete original 46-byte destructor and both relocations.
"""
from pathlib import Path
import subprocess,importlib.util,itertools
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/instruction-method-order-oct13');T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
s=(R/'vuc/probes/instruction_list_arena_unit_oct14.cpp').read_text()
s=s.replace('"../src/MLGameString.h"', '"'+str(R/'vuc/src/MLGameString.h')+'"')
markers=['void InstructionList::Add(', 'void InstructionList::AddL(', 'MLGameString InstructionList::GenLabel(',
         'MLGameString InstructionList::GenTmp(', 'void InstructionList::Print(',
         'char *Instruction::instrData;', 'const char *Instruction::ToString()',
         'void *Instruction::operator new(', 'struct InstructionArenaCleanup']
idx=[s.index(marker) for marker in markers]
assert idx==sorted(idx),idx
prefix=s[:idx[0]]
tails=s[idx[-1]:]
pieces={name:s[idx[i]:idx[i+1]] for i,name in enumerate(('Add','AddL','Label','Tmp','Print','data','ToString','new'))}
base=['Add','AddL','Label','Tmp','Print','data','ToString','new']
plans={}
def add(name,order):plans[name]=order
add('baseline',base)
for shift in range(1,len(base)):add('rotate-'+str(shift),base[shift:]+base[:shift])
for j in range(len(base)-1):
 p=list(base);p[j],p[j+1]=p[j+1],p[j];add('swap-'+str(j),p)
for name,nums in [
 ('method-original-late-data',['data','ToString','new','Add','AddL','Label','Tmp','Print']),
 ('data-first',['data','Add','AddL','Label','Tmp','Print','ToString','new']),
 ('new-first',['data','new','Add','AddL','Label','Tmp','Print','ToString']),
 ('list-reverse',['data','ToString','new','Print','Tmp','Label','AddL','Add']),
 ('list-before-data',['Print','AddL','Add','Label','Tmp','data','ToString','new']),
 ('string-funcs-first',['Label','Tmp','Add','AddL','Print','data','ToString','new']),
 ('to-string-first',['ToString','Add','AddL','Label','Tmp','Print','data','new']),
 ('new-and-tostring-first',['new','ToString','Add','AddL','Label','Tmp','Print','data']),
 ('new-before-print',['Add','AddL','Label','Tmp','new','Print','data','ToString']),
 ('print-before-add',['Print','Add','AddL','Label','Tmp','data','ToString','new']),
 ]:
 add(name,nums)
# Preserve real methods; vary construction/registration declarations after them.
scores=[]
for label,order in plans.items():
 src=T/(label+'.cpp')
 src.write_text(prefix+'\n'.join(pieces[x] for x in order)+tails)
 for flagname,flags in {
   'ren':['-O2','-frename-registers','-fuse-cxa-atexit'],
 }.items():
  out=T/(label+'-'+flagname+'.o')
  p=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*flags,'-c',str(src),'-o',str(out)],capture_output=True,text=True)
  if p.returncode:
   print('COMPILE_FAIL',label,flagname,p.stderr[-100:],flush=True);continue
  try:
   r=m.run(out,R/'vuc/config/instruction_arena_cleanup_unit_oct14_probe.json')
   scores.append((r['exact_bytes'],r['length'],label,flagname,r['different_offsets'][:5]))
   if r['exact']:
    print('NEW_EXACT',str(src),flagname,flush=True);raise SystemExit(0)
  except ValueError as e:print('REJECT',label,flagname,str(e)[:90],flush=True)
 print('TESTED',label,flush=True)
print('TOP')
for row in sorted(scores,reverse=True)[:22]:print(row)

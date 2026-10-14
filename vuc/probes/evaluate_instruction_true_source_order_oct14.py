#!/usr/bin/env python3
"""Original-function-order GCC 3.3.3 compiler-state probe for __tcf_1.

VCL earliestInsert demonstrated peep2 search-ofs persistence. Here reconstruct
the real VUC InstructionList+Instruction function order and neighboring
methods rather than artificial predecessor functions or register assembly.
"""
from pathlib import Path
import importlib.util,itertools,subprocess
ROOT=Path(__file__).resolve().parents[2]
OUT=Path('/tmp/vuc-binutils215/instruction-original-owner-oct14')
OUT.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location('m',ROOT/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
base=(ROOT/'vuc/src/InstructionListLineCommentUnit.cpp').read_text().replace(
 '#include "MLGameString.h"','#include "'+str(ROOT/'vuc/src/MLGameString.h')+'"\n#include <new>')
base=base.replace('#include <ostream>','#include <iostream>')
base=base.replace('    static void *operator new(std::size_t);','    static char *instrData;\n    static void *operator new(std::size_t);')
base=base.replace('    void AddLineComment(int line);','    void AddLineComment(int line);\n    void GenerateImage(unsigned char *);')
markers=[
 ('add','void InstructionList::Add(Instruction *instruction)'),
 ('print','void InstructionList::Print(std::ostream &output)'),
 ('addl','void InstructionList::AddL(const MLGameString &name)'),
 ('genlabel','MLGameString InstructionList::GenLabel(const char *name)'),
 ('gentmp','MLGameString InstructionList::GenTmp(const char *name)'),
 ('addv','void InstructionList::Add(const char *format, ...)'),
 ('addb','void InstructionList::AddB(const char *format, ...)'),
 ('addline','void InstructionList::AddLineComment(int line)'),
]
positions=[(n,base.index(marker)) for n,marker in markers]
original={n:base[off:(positions[i+1][1] if i+1<len(positions) else len(base))] for i,(n,off) in enumerate(positions)}
header=base[:positions[0][1]]
generate='void InstructionList::GenerateImage(unsigned char *) {}\n'
previous=(ROOT/'vuc/probes/instruction_full_source_unit_oct13.cpp').read_text()
alloc=previous[previous.index('void *Instruction::operator new(std::size_t size)'):previous.index('struct InstructionArenaCleanup')]
cleanup=previous[previous.index('struct InstructionArenaCleanup'):]
to_string='const char *Instruction::ToString() { return text.c_str(); }\n'
body={'generate':generate,'tostring':to_string,'new':alloc,**original}
order=[
 'add','addv','addb','addl','addline','generate','genlabel','gentmp','print','tostring','new',
]
# Consider legal partition points, true source-unit prefixes and a few genuine
# method reorders. The original-method order is the primary evidence source.
arrangements={
 'original':order,
 'reverse-gen':order[:5]+['generate','gentmp','genlabel']+order[8:],
 'print-before-line':order[:4]+['print','addline','generate','genlabel','gentmp','tostring','new'],
 'addl-first':['addl']+order[:3]+order[4:],
 'gen-before-line':order[:4]+['genlabel','gentmp','addline','generate','print','tostring','new'],
 'source-unit-order':['add','print','addl','genlabel','gentmp','addv','addb','addline','generate','tostring','new'],
 'instruction-first':['tostring','new']+order[:9],
}
def create(order,location,have_generate=True):
 names=[x for x in order if have_generate or x!='generate']
 all_parts=[body[n] for n in names]
 chunk='\n'.join(all_parts)
 static='char *Instruction::instrData;\n'
 if location=='begin':return header+static+chunk+cleanup
 if location=='before-new':
  offset=chunk.index('void *Instruction::operator new(std::size_t size)')
  return header+chunk[:offset]+static+chunk[offset:]+cleanup
 if location=='end':return header+chunk+static+cleanup
 raise ValueError(location)
flags={
'baseline':['-O2','-frename-registers','-fuse-cxa-atexit'],
'j1':['-O2','-frename-registers','-fuse-cxa-atexit','-falign-jumps=1'],
'j4':['-O2','-frename-registers','-fuse-cxa-atexit','-falign-jumps=4'],
'j8':['-O2','-frename-registers','-fuse-cxa-atexit','-falign-jumps=8'],
'no-regmove':['-O2','-frename-registers','-fuse-cxa-atexit','-fno-regmove'],
'cost0':['-O2','-frename-registers','-fuse-cxa-atexit','-mbranch-cost=0'],
'plain':['-O2','-fuse-cxa-atexit'],
}
rows=[]
for (name,methods),place,usegen in itertools.product(arrangements.items(),('begin','before-new','end'),(True,False)):
 key=name+'-'+place+('-full' if usegen else '-noimage')
 src=OUT/(key+'.cpp');src.write_text(create(methods,place,usegen))
 for f,opts in flags.items():
  obj=OUT/(key+'-'+f+'.o')
  p=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(src),'-o',str(obj)],text=True,capture_output=True)
  if p.returncode:
   print('COMPILE',key,f,p.stderr[-125:],flush=True);continue
  try:
   r=m.run(obj,ROOT/'vuc/config/instruction_arena_cleanup_probe.json')
   rows.append((r['exact_bytes'],r['length'],key,f,r['different_offsets'][:9]))
   if r['exact']:
    print('NEW_EXACT',src,f,flush=True);raise SystemExit(0)
  except ValueError as e:rows.append((0,0,key,f,'REJECT '+str(e)[:110]))
 print('DONE',key,flush=True)
print('TOP')
for r in sorted(rows,reverse=True)[:32]:print(r,flush=True)

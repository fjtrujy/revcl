#!/usr/bin/env python3
"""VUC AssembleBinOp: search original branch-local alignment, independent
predicate likelihood, real call-lowering in GCC 3.3.3 Jan/Dec 2003/4.

Authentic calls and copy semantics only; no machine code edits or dummy
functions. Strict ELF relocation matching for every original-length object.
"""
from pathlib import Path
import os,subprocess,importlib.util,itertools,collections
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/dispatch-align-likelihood-oct14')
T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
source=(R/'vuc/probes/assemble_binop_dispatch_partial_oct14.cpp').read_text().replace(
 '#include "../src/MLGameString.h"','#include "'+str(R/'vuc/src/MLGameString.h')+'"')
flags={
 'j2':['-O2','-frename-registers','-falign-jumps=2'],
 'j3':['-O2','-frename-registers','-falign-jumps=3'],
 'j4':['-O2','-frename-registers','-falign-jumps=4'],
 'j5':['-O2','-frename-registers','-falign-jumps=5'],
 'j6':['-O2','-frename-registers','-falign-jumps=6'],
 'j8':['-O2','-frename-registers','-falign-jumps=8'],
 'j4-l1':['-O2','-frename-registers','-falign-jumps=4','-falign-labels=1'],
 'j4-l2':['-O2','-frename-registers','-falign-jumps=4','-falign-labels=2'],
 'j4-l4':['-O2','-frename-registers','-falign-jumps=4','-falign-labels=4'],
 'j4-loop1':['-O2','-frename-registers','-falign-jumps=4','-falign-loops=1'],
 'j4-noreorder':['-O2','-frename-registers','-falign-jumps=4','-fno-reorder-blocks'],
 'j4-notrace':['-O2','-frename-registers','-falign-jumps=4','-fno-tracer'],
 'j4-cost0':['-O2','-frename-registers','-falign-jumps=4','-mbranch-cost=0'],
 'j4-no-guess':['-O2','-frename-registers','-falign-jumps=4','-fno-guess-branch-probability'],
}
snapshots={'jan25':'/tmp/vuc-binutils215/g++-jan25-as215'}
if os.environ.get('VUC_BOTH')=='1':
 snapshots['dec06']='/tmp/vuc-binutils215/g++-dec06-as215'
modes={k:v for k,v in flags.items() if k in os.environ.get('VUC_FLAGS','j3,j4,j5,j6,j8,j4-l1,j4-l2,j4-loop1,j4-cost0').split(',')}
booleans=(None,0,1)
sizes=collections.Counter();records=[];tested=0
for pred in itertools.product(booleans,repeat=4):
 src=source
 for kind,p in zip((3,4,2,1),pred):
  if p is None:
   src=src.replace(f'__builtin_expect(code == {kind}, 0)',f'code == {kind}')
  else:
   if kind==1:
    src=src.replace('if (__builtin_expect(code == 1, 0))',f'if (__builtin_expect(code == 1, {p}))')
   else:
    src=src.replace(f'if (code == {kind})',f'if (__builtin_expect(code == {kind}, {p}))')
 k=''.join('p' if p is None else str(p) for p in pred)
 cpp=T/(k+'.cpp');cpp.write_text(src)
 for snap,compiler in snapshots.items():
  for flag,opts in modes.items():
   o=T/(k+'-'+snap+'-'+flag+'.o')
   c=subprocess.run([compiler,*opts,'-c',str(cpp),'-o',str(o)],capture_output=True,text=True)
   tested+=1
   if c.returncode:
    print('COMPILE',k,snap,flag,c.stderr[-110:],flush=True);continue
   sym=next((s for s in subprocess.check_output(['nm','-S',str(o)],text=True).splitlines()
    if s.split()[-1:]==['_Z13AssembleBinOpRKN2IR8RegisterEPNS_9Expr_DataER15InstructionList']),'')
   size=int(sym.split()[1],16) if sym else -1
   sizes[size]+=1
   if size!=193:continue
   try:
    result=m.run(o,R/'vuc/config/assemble_binop_dispatch_oct14_probe.json')
    rec=(result['exact_bytes'],size,k,snap,flag,result['different_offsets'][:16])
    records.append(rec)
    if result['exact']:
     print('NEW_EXACT',cpp,snap,flag,o,flush=True);raise SystemExit(0)
   except ValueError as e:
    print('REJECT',k,snap,flag,str(e)[:110],flush=True)
  print('DONE',k,snap,'attempted',tested,'best',max((r[0] for r in records),default=0),flush=True)
print('TESTED',tested,'ORIGINAL_LENGTH',len(records),'LENGTHS',sizes.most_common(16))
print('TOP')
for row in sorted(records,reverse=True)[:40]:print(row,flush=True)

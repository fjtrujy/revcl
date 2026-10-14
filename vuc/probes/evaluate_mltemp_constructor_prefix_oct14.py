#!/usr/bin/env python3
"""VUC MLTempString::Set original constructor prefix and GCC compiler state.

Builds actual two distinct MLTempString constructors and the real Set method,
without dummy functions. Checks strict relocation and singleton-storage bytes
for the 117-byte original Set (0x08071d00).
"""
from pathlib import Path
import subprocess,importlib.util,itertools,collections,os
ROOT=Path(__file__).resolve().parents[2]; T=Path('/tmp/vuc-binutils215/mltemp-original-owner-oct14');T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location('m',ROOT/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
classsrc=(ROOT/'vuc/probes/ml_temp_constructor_unit_oct12.cpp').read_text().replace('#include "../src/MLTempStringData.h"','#include "'+str(ROOT/'vuc/src/MLTempStringData.h')+'"')
old='    MLTempString(const char *format, ...);'
assert old in classsrc
classsrc=classsrc.replace(old,old+'\n    MLTempString *Set(const char *format, ...);')
endp=classsrc.index('MLTempString::MLTempString(const char *format, ...)')
header=classsrc[:endp];ctors=classsrc[endp:]
formatctor=ctors[:ctors.index('MLTempString::MLTempString()\n{')]
defaultctor=ctors[ctors.index('MLTempString::MLTempString()\n{'):]
setsrc=(ROOT/'vuc/probes/ml_temp_set_return.cpp').read_text()
setmethod=setsrc[setsrc.index('MLTempString *MLTempString::Set('):]
methods={'formatted':formatctor,'default':defaultctor,'set':setmethod}
orders=[
['formatted','default','set'],['formatted','set','default'],
['default','formatted','set'],['default','set','formatted'],
['set','formatted','default'],['set','default','formatted'],
['formatted','set'],['default','set'],['set'],
]
flags={'plain':['-O2'],'ren':['-O2','-frename-registers'],'rj1':['-O2','-frename-registers','-falign-jumps=1'],'rj4':['-O2','-frename-registers','-falign-jumps=4'],'rj8':['-O2','-frename-registers','-falign-jumps=8'],'no-regmove':['-O2','-fno-regmove'],'no-gcse':['-O2','-fno-gcse'],'no-rename-j1':['-O2','-falign-jumps=1'],'cost0':['-O2','-mbranch-cost=0'],'no-reorder':['-O2','-fno-reorder-blocks']}
score=[];sizes=collections.Counter()
snapshot=os.environ.get('VUC_SNAPSHOT','jan25')
compiler='/tmp/vuc-binutils215/g++-'+snapshot+'-as215'
for order in orders:
 name='-'.join(order)
 cpp=T/(name+'.cpp');cpp.write_text(header+''.join(methods[x] for x in order))
 for f,opts in flags.items():
  obj=T/(name+'-'+snapshot+'-'+f+'.o')
  cc=subprocess.run([compiler,*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
  if cc.returncode:
   print('COMPILE_ERROR',name,f,cc.stderr[-140:],flush=True);continue
  nm=subprocess.check_output(['nm','-S',str(obj)],text=True)
  entry=next((s for s in nm.splitlines() if s.split()[-1:]==['_ZN12MLTempString3SetEPKcz']),'')
  length=int(entry.split()[1],16) if entry else -1;sizes[length]+=1
  if length!=117:continue
  try:
   res=m.run(obj,ROOT/'vuc/config/ml_temp_set_probe.json')
   score.append((res['exact_bytes'],length,name,f,res['different_offsets'][:15]))
   if res['exact']:
    print('NEW_EXACT',cpp,f,obj,flush=True);raise SystemExit(0)
  except ValueError as e:
   score.append((0,length,name,f,str(e)[:115]))
 print('DONE',name,flush=True)
print('LENGTHS',sizes.most_common(16))
print('TOP')
for r in sorted(score,reverse=True)[:42]:print(r,flush=True)

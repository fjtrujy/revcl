#!/usr/bin/env python3
"""VUC MLTempString::Set pointer argument aliases to reproduce real GCC 3.3.3.

The authentic earlier MLGameStringTable Add reconstruction required a named
const-pointer reference; test the same C++03 lifetime hypothesis on Set.
No artificial predecessor, register asm, bytes, or relocation targets.
"""
from pathlib import Path
import importlib.util,subprocess,itertools
ROOT=Path(__file__).resolve().parents[2]
TMP=Path('/tmp/vuc-binutils215/mltemp-set-pointer-alias-oct14');TMP.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location('m',ROOT/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
source=(ROOT/'vuc/probes/ml_temp_set_return.cpp').read_text().replace(
 '#include "../src/MLTempStringData.h"','#include "'+str(ROOT/'vuc/src/MLTempStringData.h')+'"')
start=source.index('    text = MLTempStringData::Get()->VFormatString(format, (char *)arguments);')
original='    text = MLTempStringData::Get()->VFormatString(format, (char *)arguments);'
forms={
'baseline':original,
'format-constref':'''    const char *const &fmt=format;
    text=MLTempStringData::Get()->VFormatString(fmt,(char*)arguments);''',
'format-nonconst-ref':'''    const char *&fmt=format;
    text=MLTempStringData::Get()->VFormatString(fmt,(char*)arguments);''',
'format-named':'''    const char *fmt=format;
    text=MLTempStringData::Get()->VFormatString(fmt,(char*)arguments);''',
'format-register':'''    register const char *fmt=format;
    text=MLTempStringData::Get()->VFormatString(fmt,(char*)arguments);''',
'format-and-data':'''    const char *const &fmt=format;
    MLTempStringData *data=MLTempStringData::Get();
    text=data->VFormatString(fmt,(char*)arguments);''',
'format-and-data-constref':'''    const char *const &fmt=format;
    MLTempStringData *const &data=MLTempStringData::Get();
    text=data->VFormatString(fmt,(char*)arguments);''',
'format-and-args-local':'''    const char *const &fmt=format;
    char *args=(char*)arguments;
    text=MLTempStringData::Get()->VFormatString(fmt,args);''',
'format-and-args-ref':'''    const char *const &fmt=format;
    char *const &args=(char*)arguments;
    text=MLTempStringData::Get()->VFormatString(fmt,args);''',
'format-and-thisref':'''    const char *const &fmt=format;
    MLTempString *const &self=this;
    self->text=MLTempStringData::Get()->VFormatString(fmt,(char*)arguments);''',
'format-and-result':'''    const char *const &fmt=format;
    char *result=MLTempStringData::Get()->VFormatString(fmt,(char*)arguments);
    text=result;''',
'format-and-resultconst':'''    const char *const &fmt=format;
    char *const &result=MLTempStringData::Get()->VFormatString(fmt,(char*)arguments);
    text=result;''',
'args-named':'''    char *args=(char*)arguments;
    text=MLTempStringData::Get()->VFormatString(format,args);''',
'args-constref':'''    char *const &args=(char*)arguments;
    text=MLTempStringData::Get()->VFormatString(format,args);''',
'data-constref':'''    MLTempStringData *const &data=MLTempStringData::Get();
    text=data->VFormatString(format,(char*)arguments);''',
'data-named':'''    MLTempStringData *data=MLTempStringData::Get();
    text=data->VFormatString(format,(char*)arguments);''',
'thisref':'''    MLTempString *const &self=this;
    self->text=MLTempStringData::Get()->VFormatString(format,(char*)arguments);''',
'thisref-local':'''    MLTempString &self=*this;
    self.text=MLTempStringData::Get()->VFormatString(format,(char*)arguments);''',
'result-constref':'''    char *const &result=MLTempStringData::Get()->VFormatString(format,(char*)arguments);
    text=result;''',
}
flags={
'o2':['-O2'],
'plain-j1':['-O2','-falign-jumps=1'],
'plain-j4':['-O2','-falign-jumps=4'],
'no-regmove':['-O2','-fno-regmove'],
'cost0':['-O2','-mbranch-cost=0'],
'no-sched':['-O2','-fno-schedule-insns2'],
'rename':['-O2','-frename-registers'],
}
scores=[]
for name,body in forms.items():
 cpp=TMP/(name+'.cpp');cpp.write_text(source.replace(original,body))
 for f,opts in flags.items():
  obj=TMP/(name+'-'+f+'.o')
  cc=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
  if cc.returncode:
   print('COMPILE',name,f,cc.stderr[-120:],flush=True);continue
  nm=subprocess.check_output(['nm','-S',str(obj)],text=True)
  entry=next((x for x in nm.splitlines() if x.split()[-1:]==['_ZN12MLTempString3SetEPKcz']),'')
  size=int(entry.split()[1],16) if entry else -1
  if size!=117:
   scores.append((0,size,name,f,'length'));continue
  try:
   res=m.run(obj,ROOT/'vuc/config/ml_temp_set_probe.json')
   scores.append((res['exact_bytes'],size,name,f,res['different_offsets'][:12]))
   if res['exact']:
    print('NEW_EXACT',cpp,f,flush=True);raise SystemExit(0)
  except ValueError as e:scores.append((0,size,name,f,'reject '+str(e)[:80]))
 print('DONE',name,flush=True)
print('TOP')
for x in sorted(scores,reverse=True)[:30]:print(x,flush=True)

#!/usr/bin/env python3
"""MLGameStringTable::Get exactness across genuine MLGameString unit context and constructors."""
from pathlib import Path
import subprocess, importlib.util
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/mlstringtableget-oct13');T.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location('matcher',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
base=(R/'vuc/probes/ml_string_table_get.cpp').read_text()
orig=(R/'vuc/src/MLGameString.cpp').read_text()
ctor='MLGameStringTable() : count_(0), used_(0) {}'
assert ctor in base
constructors={
 'init':ctor,
 'body': 'MLGameStringTable() { count_ = 0; used_ = 0; }',
 'reverse-body': 'MLGameStringTable() { used_ = 0; count_ = 0; }',
 'reverse-init': 'MLGameStringTable() : used_(0), count_(0) {}',
 'temp': 'MLGameStringTable() { unsigned initial = 0; count_ = initial; used_ = initial; }',
}
# Use actual source from previously exact MLGameString.cpp to establish
# historical predecessor interference; keep all virtual/type semantics.
start=orig.index('class MLGameStringTable {')
end=orig.index('MLGameStringData::MLGameStringData(')
pre=orig[:start].replace('"MLGameString.h"','"'+str(R/'vuc/src/MLGameString.h')+'"').replace('"MLTempStringData.h"','"'+str(R/'vuc/src/MLTempStringData.h')+'"')
post=orig[end:]
pre=pre.replace('MLTempString &MLTempString::operator+=', 'MLTempString &MLTempString::operator+=')
flags={'r':['-O2','-frename-registers','-fuse-cxa-atexit'],'p':['-O2','-fuse-cxa-atexit'], 'j1':['-O2','-frename-registers','-fuse-cxa-atexit','-falign-jumps=1'], 'j4':['-O2','-frename-registers','-fuse-cxa-atexit','-falign-jumps=4'],'no-sched':['-O2','-frename-registers','-fuse-cxa-atexit','-fno-schedule-insns2']}
for name,replace in constructors.items():
 model=base.replace(ctor,replace)
 source_unit=model[:model.index('MLGameStringTable *MLGameStringTable::Get()')]
 source_unit=source_unit.replace('MLGameStringData *', 'MLGameStringData *Add(const char *text, bool temporary);\n    MLGameStringData *',1) if False else source_unit
 for context in ('alone','before-real','after-real'):
  if context=='alone': code=model
  else:
   # actual member declarations must precede their source, avoid duplicate
   # MLGameString class stubs: reuse the recovered full MLGameStringTable.
   model_decl=source_unit.replace('    static MLGameStringTable *Get();','    static MLGameStringTable *Get();\n    MLGameStringData *Add(const char *text, bool temporary);')
   # Add MLGameStringData header and real model before preceding method definitions
   if context=='before-real':code=pre+model_decl+post+'\n'+model[model.index('MLGameStringTable *MLGameStringTable::Get()'):]
   else:code=pre+model_decl+model[model.index('MLGameStringTable *MLGameStringTable::Get()'):]+'\n'+post
  code=code.replace('#include "../src/MLGameString.h"','#include "'+str(R/'vuc/src/MLGameString.h')+'"')
  p=T/(name+'-'+context+'.cpp');p.write_text(code)
  for flname,opts in flags.items():
   obj=T/(name+'-'+context+'-'+flname+'.o')
   cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(p),'-o',str(obj)],capture_output=True,text=True)
   if cp.returncode:print('COMPILE-FAIL',name,context,flname,cp.stderr[-200:],flush=True);continue
   try:
    r=m.run(obj,R/'vuc/config/ml_string_table_get_probe.json')
    print(name,context,flname,f"{r['exact_bytes']}/{r['length']}",flush=True)
    if r['exact']:
     print('EXACT',p,opts,flush=True)
     raise SystemExit(0)
   except ValueError as e:print(name,context,flname,str(e)[:80],flush=True)

#!/usr/bin/env python3
"""Test real MLTempString source-neighbor definitions for original C2 register selection."""
import importlib.util,itertools,subprocess
from pathlib import Path
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/mltemp-default-context-oct13');T.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
head='''#include "'''+str(R/'vuc/src/MLTempStringData.h')+'''"
#include <cstdarg>
struct MLTempString {
 const char *text;
 MLTempString();
 void Set(const char *format, ...);
 MLTempString &operator+=(const MLTempString &other);
};
inline MLTempStringData *MLTempStringData::Get() {
 static MLTempStringData tsd;
 return &tsd;
}
'''
parts={
 'format':'''char *MLTempStringData::FormatString(const char *format, ...) {
 va_list arguments; va_start(arguments,format);
 char *result=VFormatString(format,(char *)arguments);
 va_end(arguments); return result;
}''',
 'setter':'''void MLTempString::Set(const char *format, ...) {
 va_list arguments; va_start(arguments,format);
 text=MLTempStringData::Get()->VFormatString(format,(char *)arguments);
 va_end(arguments);
}''',
 'append':'''MLTempString &MLTempString::operator+=(const MLTempString &other) {
 const char *second=other.text; const char *first=text;
 Set("%s%s",first,second); return *this;
}''',
 'default':'''MLTempString::MLTempString() {
 text=MLTempStringData::Get()->FormatString("");
}'''
}
variants=[('default',)]
for k in range(1,4):
 for others in itertools.permutations(('format','setter','append'),k):
  variants.append((*others,'default'))
  variants.append(('default',*others))
opts={'rename':['-O2','-frename-registers'],'plain':['-O2'],'align4':['-O2','-frename-registers','-falign-jumps=4'],'sections':['-O2','-frename-registers','-ffunction-sections']}
results=[]
for seq in variants:
 label='-'.join(seq)
 src=T/(label+'.cpp');src.write_text(head+'\n'.join(parts[x] for x in seq)+'\n')
 for opt,flags in opts.items():
  obj=T/(label+'-'+opt+'.o')
  cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*flags,'-c',str(src),'-o',str(obj)],capture_output=True,text=True)
  if cp.returncode:print(label,opt,'ERROR',cp.stderr[-80:],flush=True);continue
  sym=next((x for x in subprocess.run(['nm','-S',str(obj)],capture_output=True,text=True).stdout.splitlines() if '_ZN12MLTempStringC2Ev' in x),'')
  size=int(sym.split()[1],16) if sym else 0
  if size==101:
   try:
    r=m.run(obj,R/'vuc/config/ml_temp_default_c2_probe.json')
    results.append((r['exact_bytes'],label,opt))
    if r['exact']:
     print('EXACT',label,opt,src,flush=True);raise SystemExit(0)
   except ValueError as e: print(label,opt,str(e)[:80],flush=True)
  else:results.append((0,label,opt,size))
 print('checked',label,flush=True)
for row in sorted(results,reverse=True)[:24]:print('BEST',row)

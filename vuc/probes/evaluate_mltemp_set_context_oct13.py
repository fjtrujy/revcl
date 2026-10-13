#!/usr/bin/env python3
"""VUC MLTempString::Set real FormatString and constructor neighbors, GCC 3.3.3."""
import itertools,subprocess,importlib.util
from pathlib import Path
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/mltemp-set-context-oct13');T.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
header='''#include "'''+str(R/'vuc/src/MLTempStringData.h')+'''"
#include <cstdarg>
struct MLTempString {
 const char *text;
 MLTempString(const char *format, ...);
 MLTempString *Set(const char *format, ...);
};
inline MLTempStringData *MLTempStringData::Get() {
 static MLTempStringData tsd;
 return &tsd;
}
'''
parts={
 'format':'''char *MLTempStringData::FormatString(const char *format, ...) {
 va_list arguments;
 va_start(arguments, format);
 char *result=VFormatString(format,(char *)arguments);
 va_end(arguments); return result;
}''',
 'formatted':'''MLTempString::MLTempString(const char *format, ...) {
 va_list arguments;
 va_start(arguments, format);
 text=MLTempStringData::Get()->VFormatString(format,(char *)arguments);
 va_end(arguments);
}''',
 'set':'''MLTempString *MLTempString::Set(const char *format, ...) {
 va_list arguments; va_start(arguments, format);
 text=MLTempStringData::Get()->VFormatString(format,(char *)arguments);
 va_end(arguments); return this;
}''',
}
variants=[('set',)]
for k in (1,2):
 for preceding in itertools.permutations(('format','formatted'),k):
  variants.append((*preceding,'set'))
  variants.append(('set',*preceding))
flags={'r':['-O2','-frename-registers'],'j1':['-O2','-frename-registers','-falign-jumps=1'], 'j4':['-O2','-frename-registers','-falign-jumps=4'],'plain':['-O2'], 'sections':['-O2','-frename-registers','-ffunction-sections'], 'nosched':['-O2','-frename-registers','-fno-schedule-insns2']}
rank=[]
for seq in variants:
 label='-'.join(seq)
 src=T/(label+'.cpp');src.write_text(header+'\n'.join(parts[x] for x in seq)+'\n')
 for flname,opts in flags.items():
  obj=T/(label+'-'+flname+'.o')
  cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(src),'-o',str(obj)],capture_output=True,text=True)
  if cp.returncode: print('COMPILE_ERROR',label,flname,cp.stderr[-130:],flush=True);continue
  symbol=next((x for x in subprocess.run(['nm','-S',str(obj)],capture_output=True,text=True).stdout.splitlines() if '_ZN12MLTempString3SetEPKcz' in x),'')
  size=int(symbol.split()[1],16) if symbol else 0
  if size==117:
   try:
    r=m.run(obj,R/'vuc/config/ml_temp_set_probe.json')
    rank.append((r['exact_bytes'],label,flname))
    if r['exact']:
     print('EXACT',label,flname,src,flush=True);raise SystemExit(0)
   except ValueError as e:print('MATCH_ERROR',label,flname,str(e)[:150],flush=True)
  else:rank.append((0,label,flname,size))
 print('checked',label,flush=True)
for x in sorted(rank,reverse=True)[:26]:print('BEST',x)

#!/usr/bin/env python3
"""Real Value::ToString C++03 buffer sizing and branch optimizer controls.
Compare original 221-byte 8 relocations and all 82 original literal bytes;
avoid accepting wrong function lengths even with large byte overlap.
"""
from pathlib import Path
import importlib.util,itertools,json,subprocess,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
config=R/'vuc/config/value_to_string_oct13_probe.json'
src=(R/'vuc/probes/ValueToStringUnit.cpp').read_text().replace('#include "../src/MLGameString.h"','#include "'+str(R/'vuc/src/MLGameString.h')+'"')
srcpart=src[src.index('MLGameString Value::ToString() const'):]
assert 'char buffer[64] = "";' in srcpart
forms={
 'init-empty':'char buffer[{n}] = "";',
 'init-zero':'char buffer[{n}] = {{0}};',
 'init-null':'char buffer[{n}] = "\\0";',
}
flags={
 'saved':['-O2','-frename-registers','-falign-jumps=4'],
 'j1':['-O2','-frename-registers','-falign-jumps=1'],
 'j8':['-O2','-frename-registers','-falign-jumps=8'],
 'j4l2':['-O2','-frename-registers','-falign-jumps=4','-falign-labels=2'],
 'no-cross':['-O2','-frename-registers','-falign-jumps=4','-fno-crossjumping'],
 'no-reorder':['-O2','-frename-registers','-falign-jumps=4','-fno-reorder-blocks'],
 'plain':['-O2','-falign-jumps=4'],
 'j4loops':['-O2','-frename-registers','-falign-jumps=4','-falign-loops=1'],
}
T=Path('/tmp/vuc-binutils215/value-tostring-buffer-real-oct15c');T.mkdir(parents=True,exist_ok=True)
results=[];sizes=collections.Counter();best=None
for n,(f,form),(fl,opts) in itertools.product((32,40,48,56,60,64,68,72,80,96,128),forms.items(),flags.items()):
 tag=f'n{n}-{f}-{fl}';source=src.replace('char buffer[64] = "";',form.format(n=n))
 cpp=T/(tag+'.cpp');obj=T/(tag+'.o');cpp.write_text(source)
 p=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if p.returncode:results.append(dict(tag=tag,error=p.stderr[-140:]));continue
 try:d=m.run(obj,config)
 except ValueError as e:
  symbol=next((line for line in subprocess.check_output(['nm','-S',str(obj)],text=True).splitlines() if line.endswith('_ZNK5Value8ToStringEv')),None)
  length=int(symbol.split()[1],16) if symbol else 0;sizes[length]+=1
  results.append(dict(tag=tag,length=length,error=str(e)[:130]));continue
 row=dict(tag=tag,length=d['length'],match=d['exact_bytes'],relocations=d['relocations'],literals=d['verified_literal_bytes'],prefix=next(iter(d['different_offsets']),d['length']))
 results.append(row);sizes[row['length']]+=1
 if row['length']==221 and row['relocations']==8 and row['literals']==82 and (best is None or (row['match'],row['prefix'])>(best['match'],best['prefix'])):
  best=row;(T/'best.cpp').write_text(source);(T/'best.o').write_bytes(obj.read_bytes());(T/'best.flags').write_text(' '.join(opts)+'\n')
(T/'results.json').write_text(json.dumps(results,indent=2)+'\n')
print(json.dumps(dict(attempted=len(results),sizes=dict(sizes),best=best,top=sorted((r for r in results if 'match' in r),key=lambda r:(r['match'],r['prefix']),reverse=True)[:16]),indent=2))

#!/usr/bin/env python3
"""Recover original VUC Value::Set stack ownership with plausible temporary buffer capacities.
Test actual C++03 buffer sizes / name lifetime and genuine period GCC settings,
not assembler padding. Record original-size + named-relocation + literal byte checks.
"""
from pathlib import Path
import importlib.util,json,itertools,subprocess,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
config=json.loads((R/'vuc/config/value_set_constval_partial_oct15b.json').read_text())
orig,elf=m.decomp.load();s=next(x for x in elf['sections'] if x['name']=='.text')
addr=int(config['address'],0);ref=orig[s['offset']+addr-s['address']:s['offset']+addr-s['address']+config['size']]
src=(R/'vuc/probes/ValueConstValSetPartialOct15b.cpp').read_text().replace('#include "../src/MLGameString.h"','#include "'+str(R/'vuc/src/MLGameString.h')+'"')
capacities=[48,56,64,72,76,80,84,88,92,96,100,104,112,120,128,132,144,160]
flags={
 'saved':['-O2','-frename-registers','-falign-jumps=4','-falign-labels=2','-falign-loops=1','-fno-reorder-blocks'],
 'j1':['-O2','-frename-registers','-falign-jumps=1'],
 'j4':['-O2','-frename-registers','-falign-jumps=4'],
 'plain':['-O2'],
 'noreorder':['-O2','-frename-registers','-falign-jumps=1','-fno-reorder-blocks'],
 'j4l2':['-O2','-frename-registers','-falign-jumps=4','-falign-labels=2'],
}
forms={
 'ordinary':"        char scratch[100];",
 'unsignedbuf':"        unsigned char scratch[100];",
 'volatilebuf':"        volatile char scratch[100];",
 'shortbuf':"        char scratch[100] = {0};",
 'scopedbuf':"        char scratch[100];",
}
T=Path('/tmp/vuc-binutils215/value-set-scratch-size-oct15c');T.mkdir(parents=True,exist_ok=True)
results=[];sizes=collections.Counter();best=None
for capacity,(form,decl),(fl,opts) in itertools.product(capacities,forms.items(),flags.items()):
 if form in ('unsignedbuf','volatilebuf'):continue # C++ ABI compiler type conversion to memcpy problematic
 tag=f'n{capacity}-{form}-{fl}'
 source=src.replace('        char scratch[100];',decl.replace('[100]',f'[{capacity}]'))
 if source==src and capacity!=100:raise ValueError('no source variation')
 cpp=T/(tag+'.cpp');obj=T/(tag+'.o');cpp.write_text(source)
 cc=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if cc.returncode:results.append(dict(tag=tag,error=cc.stderr[-140:]));continue
 try:code,rel=m.prepare_named_function(obj.read_bytes(),config['function'],config['relocations'],addr,literal_ranges=config['relocation_literals'])
 except ValueError as e:results.append(dict(tag=tag,error=str(e)));continue
 score=sum(a==b for a,b in zip(code,ref));prefix=next((i for i,(a,b) in enumerate(zip(code,ref)) if a!=b),min(len(code),len(ref)))
 frame=code[5:9].hex() if len(code)>9 else None
 row=dict(tag=tag,length=len(code),relocations=rel,match=score,prefix=prefix,frame=frame)
 results.append(row);sizes[len(code)]+=1
 if len(code)==config['size'] and rel==13 and (best is None or (score,prefix)>(best['match'],best['prefix'])):
  best=row;(T/'best.cpp').write_text(source);(T/'best.flags').write_text(' '.join(opts)+'\n');(T/'best.o').write_bytes(obj.read_bytes())
(T/'results.json').write_text(json.dumps(results,indent=2)+'\n')
print(json.dumps(dict(attempted=len(results),sizes=dict(sizes),best=best,leading=sorted((r for r in results if 'match' in r),key=lambda x:(x['match'],x['prefix']),reverse=True)[:15]),indent=2))

#!/usr/bin/env python3
"""VUC Value::Set 455-byte target after authentic fallback recovered with -fno-crossjumping.
Require all 15 real references (6 c_str, 2 strtol, 3 strtod, strrchr,
memcpy and two boolean literals). Search GCC 3.3.3 branch/label/loop
alignment for original 455-byte allocation without fake source padding.
"""
from pathlib import Path
import itertools,json,subprocess,importlib.util,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
conf=json.loads((R/'vuc/config/value_set_constval_partial_oct15b.json').read_text())
raw,elf=m.decomp.load();s=next(x for x in elf['sections'] if x['name']=='.text');addr=int(conf['address'],0)
ref=raw[s['offset']+addr-s['address']:s['offset']+addr-s['address']+455]
base=(R/'vuc/probes/ValueConstValSetPartialOct15b.cpp').read_text().replace('#include "../src/MLGameString.h"','#include "'+str(R/'vuc/src/MLGameString.h')+'"')
T=Path('/tmp/vuc-binutils215/value-set-no-crossjump-align-oct15c');T.mkdir(parents=True,exist_ok=True)
records=[];counts=collections.Counter();best=None
for buffer,rename,jmp,label,loop in itertools.product((56,64,80),(True,False),(1,2,3,4,5,8,16),(1,2,3,4,8),(1,2,4)):
 tag=f'n{buffer}-{"r" if rename else "p"}-j{jmp}-l{label}-loop{loop}'
 flags=['-O2']+(['-frename-registers'] if rename else [])+[f'-falign-jumps={jmp}',f'-falign-labels={label}',f'-falign-loops={loop}','-fno-crossjumping']
 code=base.replace('char scratch[100]',f'char scratch[{buffer}]')
 cpp=T/(tag+'.cpp');obj=T/(tag+'.o');cpp.write_text(code)
 p=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*flags,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if p.returncode:records.append(dict(tag=tag,error=p.stderr[-120:]));continue
 try:b,rel=m.prepare_named_function(obj.read_bytes(),conf['function'],conf['relocations'],addr,literal_ranges=conf['relocation_literals'])
 except ValueError as e:records.append(dict(tag=tag,error=str(e)));continue
 match=sum(x==y for x,y in zip(b,ref))
 prefix=next((i for i,(x,y) in enumerate(zip(b,ref)) if x!=y),min(len(b),len(ref)))
 row=dict(tag=tag,length=len(b),relocations=rel,match=match,prefix=prefix)
 records.append(row);counts[len(b)]+=1
 if len(b)==455 and rel==15 and (best is None or (match,prefix)>(best['match'],best['prefix'])):
  best=row;(T/'best.cpp').write_text(code);(T/'best.o').write_bytes(obj.read_bytes());(T/'best.flags').write_text(' '.join(flags)+'\n')
(T/'results.json').write_text(json.dumps(records,indent=2)+'\n')
print(json.dumps(dict(attempted=len(records),sizes=dict(counts),best=best,top=sorted((r for r in records if 'match' in r),key=lambda r:(r['match'],r['prefix']),reverse=True)[:14]),indent=2))

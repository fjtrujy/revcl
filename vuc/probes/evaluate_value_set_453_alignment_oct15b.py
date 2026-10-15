#!/usr/bin/env python3
"""Recover strict original-length VUC Value::Set constval from 453-byte source.
The C++03 two-sources boolean form independently calls c_str(), preserving
the original observed call graph. Test historical branch/label/loop flags.
Only original-sized 455-byte result with authentic symbols/literals is valid.
"""
from pathlib import Path
import importlib.util,json,itertools,subprocess,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=R/'vuc/config/value_set_constval_investigation_oct15b.json'
source=(R/'vuc/probes/ValueConstValSetPartialOct15b.cpp').read_text().replace(
    '#include "../src/MLGameString.h"',
    '#include "'+str(R/'vuc/src/MLGameString.h')+'"')
T=Path('/tmp/vuc-binutils215/vuc-value-set-453-align-oct15b');T.mkdir(parents=True,exist_ok=True)
ranks=[];counts=collections.Counter();best=None
for rename,jump,label,loop in itertools.product((False,True),(1,2,4,8,16,32),(1,2,4,8,16),(1,2,4)):
 tag=f'{"ren" if rename else "plain"}-j{jump}-l{label}-loop{loop}'
 opts=['-O2']+(['-frename-registers'] if rename else [])+[f'-falign-jumps={jump}',f'-falign-labels={label}',f'-falign-loops={loop}']
 cpp=T/(tag+'.cpp');obj=T/(tag+'.o');cpp.write_text(source)
 cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if cp.returncode:ranks.append(dict(tag=tag,error=cp.stderr[-120:]));continue
 symbol=next((line for line in subprocess.check_output(['nm','-S',str(obj)],text=True).splitlines() if line.endswith('_ZN5Value3SetEPK15ASTConstValExpr')),None)
 n=int(symbol.split()[1],16) if symbol else 0;counts[n]+=1
 if n!=455:ranks.append(dict(tag=tag,length=n));continue
 try:score=m.run(obj,cfg)
 except ValueError as e:ranks.append(dict(tag=tag,length=n,error=str(e)[:200]));continue
 row=dict(tag=tag,length=n,match=score['exact_bytes'],relocations=score['relocations'],prefix=next(iter(score['different_offsets']),n))
 ranks.append(row)
 if best is None or (row['match'],row['prefix'])>(best['match'],best['prefix']):
  best=row;(T/'best.cpp').write_text(source);(T/'best.o').write_bytes(obj.read_bytes());(T/'best.flags').write_text(' '.join(opts)+'\n')
(T/'results.json').write_text(json.dumps(ranks,indent=2)+'\n')
print(json.dumps(dict(attempted=len(ranks),sizes=dict(counts),best=best,exactsize=[x for x in ranks if x.get('length')==455][:15]),indent=2))

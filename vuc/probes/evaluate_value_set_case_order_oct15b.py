#!/usr/bin/env python3
"""VUC original ASTConstValExpr->Value lexical switch case ownership.
Recover the original 455-byte compiler block order with genuine type 1
integer, 2 boolean, 3 scalar, and 4 vector parser arms, full real relocation
and two literal source checks. No source padding, asm or synthetic calls.
"""
from pathlib import Path
import importlib.util,itertools,json,subprocess,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=R/'vuc/config/value_set_constval_partial_oct15b.json'
base=(R/'vuc/probes/ValueConstValSetPartialOct15b.cpp').read_text().replace(
 '#include "../src/MLGameString.h"','#include "'+str(R/'vuc/src/MLGameString.h')+'"')
marker=['    case 1:', '    case 2:', '    case 3:', '    case 4:']
a=base.index(marker[0]);z=base.index('\n    }\n}',base.index(marker[3]))
points=[base.index(t,a) for t in marker]+[z]
arms={i+1:base[points[i]:points[i+1]] for i in range(4)}
flags={
 'saved':['-O2','-frename-registers','-falign-jumps=4','-falign-labels=2','-falign-loops=1','-fno-reorder-blocks'],
 'default':['-O2','-frename-registers','-falign-jumps=1'],
 'j4l2':['-O2','-frename-registers','-falign-jumps=4','-falign-labels=2'],
 'j4':['-O2','-frename-registers','-falign-jumps=4'],
 'j8':['-O2','-frename-registers','-falign-jumps=8'],
 'j4l1':['-O2','-frename-registers','-falign-jumps=4','-falign-labels=1'],
 'j1l2':['-O2','-frename-registers','-falign-jumps=1','-falign-labels=2'],
 'j4l4':['-O2','-frename-registers','-falign-jumps=4','-falign-labels=4'],
 'plain':['-O2'],
 'plainj4l2':['-O2','-falign-jumps=4','-falign-labels=2'],
}
T=Path('/tmp/vuc-binutils215/vuc-value-set-case-lexical-oct15b');T.mkdir(parents=True,exist_ok=True)
rows=[];sizes=collections.Counter();best=None
for order,(fl,opts) in itertools.product(itertools.permutations((1,2,3,4)),flags.items()):
 tag=''.join(map(str,order))+'-'+fl
 code=base[:a]+''.join(arms[i] for i in order)+base[z:]
 cpp=T/(tag+'.cpp');obj=T/(tag+'.o');cpp.write_text(code)
 cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if cp.returncode:rows.append(dict(tag=tag,error=cp.stderr[-115:]));continue
 symbol=next((line for line in subprocess.check_output(['nm','-S',str(obj)],text=True).splitlines() if line.endswith('_ZN5Value3SetEPK15ASTConstValExpr')),None)
 n=int(symbol.split()[1],16) if symbol else 0;sizes[n]+=1
 if n!=455:rows.append(dict(tag=tag,length=n));continue
 try:r=m.run(obj,cfg)
 except ValueError as e:rows.append(dict(tag=tag,length=n,error=str(e)[:190]));continue
 row=dict(tag=tag,length=n,match=r['exact_bytes'],relocations=r['relocations'],literal_bytes=r['verified_literal_bytes'],prefix=next(iter(r['different_offsets']),n))
 rows.append(row)
 if best is None or (row['match'],row['prefix'])>(best['match'],best['prefix']):
  best=row;(T/'best.cpp').write_text(code);(T/'best.o').write_bytes(obj.read_bytes());(T/'best.flags').write_text(' '.join(opts)+'\n')
(T/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
print(json.dumps(dict(attempted=len(rows),sizes=dict(sizes),best=best,top=sorted((r for r in rows if 'match' in r),key=lambda r:(r['match'],r['prefix']),reverse=True)[:12]),indent=2))

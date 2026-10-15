#!/usr/bin/env python3
"""Original VUC Value::Set AST parser: 64-byte local buffer and lexical case order.
Valid historical C++03 source only. Require 455-byte function, 13 original
relocations and both original boolean literal bytes before promoting.
"""
from pathlib import Path
import itertools,json,importlib.util,subprocess,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=R/'vuc/config/value_set_constval_partial_oct15b.json'
source=(R/'vuc/probes/ValueConstValSetPartialOct15b.cpp').read_text()
source=source.replace('#include "../src/MLGameString.h"','#include "'+str(R/'vuc/src/MLGameString.h')+'"').replace('char scratch[100]','char scratch[64]')
start=source.index('    case 1:');end=source.index('\n    }\n}',source.index('    case 4:'))
points=[source.index(f'    case {n}:',start) for n in range(1,5)]+[end]
arms={n:source[points[n-1]:points[n]] for n in range(1,5)}
flags={
 'saved':['-O2','-frename-registers','-falign-jumps=4','-falign-labels=2','-falign-loops=1','-fno-reorder-blocks'],
 'j1':['-O2','-frename-registers','-falign-jumps=1'],
 'j4':['-O2','-frename-registers','-falign-jumps=4'],
 'j8':['-O2','-frename-registers','-falign-jumps=8'],
 'j4l2':['-O2','-frename-registers','-falign-jumps=4','-falign-labels=2'],
 'l4':['-O2','-frename-registers','-falign-jumps=4','-falign-labels=4'],
 'plain':['-O2'],
 'plain-no-reorder':['-O2','-fno-reorder-blocks'],
}
T=Path('/tmp/vuc-binutils215/value-set-buffer64-caseorder-oct15c');T.mkdir(parents=True,exist_ok=True)
rows=[];sizes=collections.Counter();best=None
for order,(fn,opts) in itertools.product(itertools.permutations((1,2,3,4)),flags.items()):
 tag=''.join(map(str,order))+'-'+fn
 code=source[:start]+''.join(arms[n] for n in order)+source[end:]
 cpp=T/(tag+'.cpp');obj=T/(tag+'.o');cpp.write_text(code)
 p=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if p.returncode:rows.append(dict(tag=tag,error=p.stderr[-160:]));continue
 nline=next((line for line in subprocess.check_output(['nm','-S',str(obj)],text=True).splitlines() if line.endswith('_ZN5Value3SetEPK15ASTConstValExpr')),None)
 size=int(nline.split()[1],16) if nline else -1;sizes[size]+=1
 if size!=455:rows.append(dict(tag=tag,length=size));continue
 try:d=m.run(obj,cfg)
 except ValueError as e:rows.append(dict(tag=tag,length=size,error=str(e)[:180]));continue
 row=dict(tag=tag,length=size,match=d['exact_bytes'],relocations=d['relocations'],literal_bytes=d['verified_literal_bytes'],prefix=next(iter(d['different_offsets']),size))
 rows.append(row)
 if best is None or (row['match'],row['prefix'])>(best['match'],best['prefix']):
  best=row;(T/'best.cpp').write_text(code);(T/'best.o').write_bytes(obj.read_bytes());(T/'best.flags').write_text(' '.join(opts)+'\n')
(T/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
print(json.dumps(dict(attempted=len(rows),sizes=dict(sizes),best=best,top=sorted((r for r in rows if 'match' in r),key=lambda r:(r['match'],r['prefix']),reverse=True)[:12]),indent=2))

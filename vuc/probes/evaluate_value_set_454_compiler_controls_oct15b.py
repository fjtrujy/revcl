#!/usr/bin/env python3
"""VUC 455-byte const-value parser: 454-byte authentic source compiler controls.
No source padding or x86 instructions. Require 455 original bytes and
13 named/literal relocations verified by existing match_probe strict gate.
"""
from pathlib import Path
import importlib.util,json,itertools,subprocess,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=R/'vuc/config/value_set_constval_investigation_oct15b.json'
source=(R/'vuc/probes/ValueConstValSetPartialOct15b.cpp').read_text().replace(
    '#include "../src/MLGameString.h"',
    '#include "'+str(R/'vuc/src/MLGameString.h')+'"')
controls={
 'baseline':[],
 'j3':['-falign-jumps=3'],
 'j5':['-falign-jumps=5'],
 'j6':['-falign-jumps=6'],
 'j7':['-falign-jumps=7'],
 'l3':['-falign-labels=3'],
 'l5':['-falign-labels=5'],
 'l6':['-falign-labels=6'],
 'l7':['-falign-labels=7'],
 'loop3':['-falign-loops=3'],
 'loop5':['-falign-loops=5'],
 'loop8':['-falign-loops=8'],
 'no-gcse':['-fno-gcse'],
 'no-csefollow':['-fno-cse-follow-jumps'],
 'no-cseskip':['-fno-cse-skip-blocks'],
 'no-sched1':['-fno-schedule-insns'],
 'no-sched2':['-fno-schedule-insns2'],
 'no-regmove':['-fno-regmove'],
 'no-ifconversion':['-fno-if-conversion'],
 'no-ifconversion2':['-fno-if-conversion2'],
 'no-peephole2':['-fno-peephole2'],
 'no-reorder':['-fno-reorder-blocks'],
 'no-crossjump':['-fno-crossjumping'],
 'no-guess':['-fno-guess-branch-probability'],
 'no-thread':['-fno-thread-jumps'],
 'no-cprop':['-fno-cprop-registers'],
 'no-builtin':['-fno-builtin'],
 'no-inline':['-fno-inline'],
 'no-strict':['-fno-strict-aliasing'],
 'no-deferpop':['-fno-defer-pop'],
 'cost0':['-mbranch-cost=0'],
 'cost1':['-mbranch-cost=1'],
 'cost2':['-mbranch-cost=2'],
 'cost3':['-mbranch-cost=3'],
 'cost4':['-mbranch-cost=4'],
 'stack2':['-mpreferred-stack-boundary=2'],
 'cpu386':['-mcpu=i386'],
 'cpu486':['-mcpu=i486'],
 'cpupentium':['-mcpu=pentium'],
 'arch386':['-march=i386'],
 'arch486':['-march=i486'],
}
T=Path('/tmp/vuc-binutils215/vuc-value-set-454-pass-oct15b');T.mkdir(parents=True,exist_ok=True)
rows=[];sizes=collections.Counter();best=None
for (key,control),snapshot in itertools.product(controls.items(),('jan25','dec06')):
 tag=f'{key}-{snapshot}'
 flags=['-O2','-frename-registers','-falign-jumps=4','-falign-labels=2','-falign-loops=1',*control]
 cpp=T/(tag+'.cpp');obj=T/(tag+'.o');cpp.write_text(source)
 p=subprocess.run(['/tmp/vuc-binutils215/g++-'+snapshot+'-as215',*flags,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if p.returncode:rows.append(dict(tag=tag,error=p.stderr[-120:]));continue
 symbol=next((line for line in subprocess.check_output(['nm','-S',str(obj)],text=True).splitlines() if line.endswith('_ZN5Value3SetEPK15ASTConstValExpr')),None)
 n=int(symbol.split()[1],16) if symbol else 0;sizes[n]+=1
 if n!=455:rows.append(dict(tag=tag,length=n));continue
 try:res=m.run(obj,cfg)
 except ValueError as e:rows.append(dict(tag=tag,length=n,error=str(e)[:170]));continue
 row=dict(tag=tag,length=n,match=res['exact_bytes'],relocations=res['relocations'],prefix=next(iter(res['different_offsets']),n))
 rows.append(row)
 if best is None or (row['match'],row['prefix'])>(best['match'],best['prefix']):
  best=row;(T/'best.cpp').write_text(source);(T/'best.flags').write_text(' '.join(flags)+'\n');(T/'best.o').write_bytes(obj.read_bytes())
(T/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
print(json.dumps(dict(attempted=len(rows),sizes=dict(sizes),best=best,strict_original_length=[r for r in rows if r.get('length')==455][:20]),indent=2))

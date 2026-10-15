#!/usr/bin/env python3
"""Historical GCC 3.3.3 real control-flow pass experiment for VUC Value Set.
The original boolean fallback has separate strtol path; candidate merges it
with integer case and becomes 422 bytes. Look for genuine -fno-crossjumping
and optimization options producing authentic 455-byte C++03 source.
"""
from pathlib import Path
import itertools,json,importlib.util,subprocess,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=R/'vuc/config/value_set_constval_partial_oct15b.json'
base=(R/'vuc/probes/ValueConstValSetPartialOct15b.cpp').read_text().replace('#include "../src/MLGameString.h"','#include "'+str(R/'vuc/src/MLGameString.h')+'"')
controls={
 'base':[],
 'no-crossjump':['-fno-crossjumping'],
 'no-thread':['-fno-thread-jumps'],
 'no-cse-follow':['-fno-cse-follow-jumps'],
 'no-cse-skip':['-fno-cse-skip-blocks'],
 'no-gcse':['-fno-gcse'],
 'no-cprop':['-fno-cprop-registers'],
 'no-regmove':['-fno-regmove'],
 'no-peephole2':['-fno-peephole2'],
 'no-sched2':['-fno-schedule-insns2'],
 'no-sched1':['-fno-schedule-insns'],
 'no-guess':['-fno-guess-branch-probability'],
 'no-ifconv':['-fno-if-conversion'],
 'no-ifconv2':['-fno-if-conversion2'],
 'no-reorder':['-fno-reorder-blocks'],
 'no-opt-sib':['-fno-optimize-sibling-calls'],
 'no-deferpop':['-fno-defer-pop'],
 'no-inline':['-fno-inline'],
 'no-builtin':['-fno-builtin'],
 'no-strict':['-fno-strict-aliasing'],
 'no-expensive':['-fno-expensive-optimizations'],
 'no-rerun-cse':['-fno-rerun-cse-after-loop'],
 'cost0':['-mbranch-cost=0'],
 'cost1':['-mbranch-cost=1'],
 'cost2':['-mbranch-cost=2'],
 'cost3':['-mbranch-cost=3'],
 'cost4':['-mbranch-cost=4'],
}
flags={
 'j1':['-O2','-frename-registers','-falign-jumps=1'],
 'j4':['-O2','-frename-registers','-falign-jumps=4'],
 'saved':['-O2','-frename-registers','-falign-jumps=4','-falign-labels=2','-falign-loops=1','-fno-reorder-blocks'],
 'j4l2':['-O2','-frename-registers','-falign-jumps=4','-falign-labels=2'],
}
T=Path('/tmp/vuc-binutils215/value-set-buffer64-crossjump-oct15c');T.mkdir(parents=True,exist_ok=True)
results=[];sizes=collections.Counter();best=None
for buffer,(f,extra),(fn,opts),snapshot in itertools.product((56,64,72,80,96),controls.items(),flags.items(),('jan25','dec06')):
 tag=f'n{buffer}-{f}-{fn}-{snapshot}'
 code=base.replace('char scratch[100]',f'char scratch[{buffer}]')
 cpp=T/(tag+'.cpp');obj=T/(tag+'.o');cpp.write_text(code)
 r=subprocess.run(['/tmp/vuc-binutils215/g++-'+snapshot+'-as215',*opts,*extra,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if r.returncode:results.append(dict(tag=tag,error=r.stderr[-125:]));continue
 symbol=next((line for line in subprocess.check_output(['nm','-S',str(obj)],text=True).splitlines() if line.endswith('_ZN5Value3SetEPK15ASTConstValExpr')),None)
 n=int(symbol.split()[1],16) if symbol else -1;sizes[n]+=1
 if n!=455:results.append(dict(tag=tag,length=n));continue
 try:d=m.run(obj,cfg)
 except ValueError as e:results.append(dict(tag=tag,length=n,error=str(e)[:160]));continue
 row=dict(tag=tag,length=n,match=d['exact_bytes'],relocations=d['relocations'],literal_bytes=d['verified_literal_bytes'],prefix=next(iter(d['different_offsets']),n))
 results.append(row)
 if best is None or (row['match'],row['prefix'])>(best['match'],best['prefix']):
  best=row;(T/'best.cpp').write_text(code);(T/'best.o').write_bytes(obj.read_bytes());(T/'best.flags').write_text(' '.join([*opts,*extra])+'\n')
(T/'results.json').write_text(json.dumps(results,indent=2)+'\n')
print(json.dumps(dict(attempted=len(results),sizes=dict(sizes),best=best,top=sorted((r for r in results if 'match' in r),key=lambda r:(r['match'],r['prefix']),reverse=True)[:20]),indent=2))

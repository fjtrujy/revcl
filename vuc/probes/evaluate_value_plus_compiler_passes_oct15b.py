#!/usr/bin/env python3
"""Value::operator+ original-length GCC3.3.3 RTL/peephole pass and CPU controls.
Test the best 203/219 lane order alongside byte-identical 216-byte natural
source. Genuine historical compiler target knobs only, never asm or padding.
"""
from pathlib import Path
import importlib.util,json,itertools,subprocess,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=json.loads((R/'vuc/config/value_addition_partial_oct14_probe.json').read_text())
raw,elf=m.decomp.load();sec=next(x for x in elf['sections'] if x['name']=='.text');address=int(cfg['address'],0)
ref=raw[sec['offset']+address-sec['address']:sec['offset']+address-sec['address']+219]
base=(R/'vuc/probes/ValueAdditionLaneOrderOct15.cpp').read_text()
selected='\n'.join(f'            result.values[{i}] += temporary.values[{i}];' for i in (3,1,2,0))
orders={'3120':(3,1,2,0),'1320':(1,3,2,0),'0123':(0,1,2,3),'3210':(3,2,1,0)}
options={
 'baseline':[],
 'no-reorder':['-fno-reorder-blocks'],
 'no-crossjump':['-fno-crossjumping'],
 'no-thread':['-fno-thread-jumps'],
 'no-cse-follow':['-fno-cse-follow-jumps'],
 'no-cse-skip':['-fno-cse-skip-blocks'],
 'no-gcse':['-fno-gcse'],
 'no-regmove':['-fno-regmove'],
 'no-sched2':['-fno-schedule-insns2'],
 'no-sched1':['-fno-schedule-insns'],
 'no-guess':['-fno-guess-branch-probability'],
 'no-if-conv':['-fno-if-conversion'],
 'no-if-conv2':['-fno-if-conversion2'],
 'no-peephole2':['-fno-peephole2'],
 'no-peephole':['-fno-peephole'],
 'no-web':['-fno-web'],
 'no-cprop-registers':['-fno-cprop-registers'],
 'no-caller-saves':['-fno-caller-saves'],
 'no-strict-alias':['-fno-strict-aliasing'],
 'no-ivopts':['-fno-ivopts'],
 'no-builtin':['-fno-builtin'],
 'no-fun-cse':['-fno-function-cse'],
 'no-inline':['-fno-inline'],
 'expensive-off':['-fno-expensive-optimizations'],
 'deferpop-off':['-fno-defer-pop'],
 'no-opt-sibling':['-fno-optimize-sibling-calls'],
 'no-omit-frame':['-fno-omit-frame-pointer'],
 'branch-cost0':['-mbranch-cost=0'],
 'branch-cost1':['-mbranch-cost=1'],
 'branch-cost2':['-mbranch-cost=2'],
 'branch-cost3':['-mbranch-cost=3'],
 'branch-cost4':['-mbranch-cost=4'],
 'cpu-i386':['-mcpu=i386'],
 'cpu-i486':['-mcpu=i486'],
 'cpu-pentium':['-mcpu=pentium'],
 'cpu-pentiumpro':['-mcpu=pentiumpro'],
 'cpu-i686':['-mcpu=i686'],
 'arch-i386':['-march=i386'],
 'arch-i486':['-march=i486'],
 'arch-pentium':['-march=pentium'],
 'arch-i686':['-march=i686'],
 'align-loops1':['-falign-loops=1'],
 'align-loops2':['-falign-loops=2'],
 'align-loops4':['-falign-loops=4'],
 'align-loops8':['-falign-loops=8'],
 'align-labels1':['-falign-labels=1'],
 'align-labels2':['-falign-labels=2'],
 'align-labels4':['-falign-labels=4'],
 'align-labels8':['-falign-labels=8'],
 'align-labels16':['-falign-labels=16'],
 'align-jumps2':['-falign-jumps=2'],
 'align-jumps4':['-falign-jumps=4'],
}
T=Path('/tmp/vuc-binutils215/vuc-plus-compiler-pass-oct15b');T.mkdir(parents=True,exist_ok=True)
rows=[];sizes=collections.Counter();best=None
for (order,seq),(name,extra),snapshot in itertools.product(orders.items(),options.items(),('jan25','dec06')):
 tag=f'{order}-{name}-{snapshot}'
 body=base.replace(selected,'\n'.join(f'            result.values[{i}] += temporary.values[{i}];' for i in seq))
 obj=T/(tag+'.o');cpp=T/(tag+'.cpp');cpp.write_text(body)
 opts=['-O2','-frename-registers','-falign-jumps=1',*extra]
 proc=subprocess.run(['/tmp/vuc-binutils215/g++-'+snapshot+'-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if proc.returncode:rows.append(dict(tag=tag,error=proc.stderr[-120:]));continue
 try:b,rel=m.prepare_named_function(obj.read_bytes(),cfg['function'],cfg['relocations'],address)
 except ValueError as e:rows.append(dict(tag=tag,error=str(e)));continue
 overlap=sum(x==y for x,y in zip(ref,b));pfx=next((i for i,(x,y) in enumerate(zip(ref,b)) if x!=y),min(len(ref),len(b)))
 row=dict(tag=tag,length=len(b),relocations=rel,match=overlap,prefix=pfx)
 rows.append(row);sizes[len(b)]+=1
 if len(b)==219 and rel==1 and (best is None or (overlap,pfx)>(best['match'],best['prefix'])):
  best=row;(T/'best.cpp').write_text(body);(T/'best.flags').write_text(' '.join(opts)+'\n');(T/'best.o').write_bytes(obj.read_bytes())
(T/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
print(json.dumps(dict(attempted=len(rows),sizes=dict(sizes),best=best,top=sorted((r for r in rows if 'match' in r),key=lambda r:(r['match'],r['prefix']),reverse=True)[:13]),indent=2))

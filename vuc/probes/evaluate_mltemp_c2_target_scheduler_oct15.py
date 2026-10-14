#!/usr/bin/env python3
"""Historical GCC 3.3.3 original MLTempString default C2 register lifetime.

The accepted C1 constructor is 102/102, but original C2 is 101 bytes;
the current original-sized candidate hits 95/101 with six register-lifetime
differences. Probe real compiler target/scheduler switches and two authentic
constructor source contexts; never force concrete x86 registers or bytes.
"""
from pathlib import Path
import importlib.util,itertools,subprocess,json,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=R/'vuc/config/ml_temp_default_C2_probe.json'
contexts={
 'isolated':R/'vuc/probes/ml_temp_default.cpp',
 'real-neighbors':R/'vuc/probes/ml_temp_constructor_unit_oct12.cpp',
 'exact-C1-owner':R/'vuc/src/MLTempStringDefault.cpp',
}
flags={
 'baseline':['-O2','-frename-registers'],
 'plain':['-O2'],
 'j1':['-O2','-frename-registers','-falign-jumps=1'],
 'j4':['-O2','-frename-registers','-falign-jumps=4'],
 'j8':['-O2','-frename-registers','-falign-jumps=8'],
 'j16':['-O2','-frename-registers','-falign-jumps=16'],
 'no-sched':['-O2','-frename-registers','-fno-schedule-insns2'],
 'no-sched1':['-O2','-frename-registers','-fno-schedule-insns'],
 'no-cprop':['-O2','-frename-registers','-fno-cprop-registers'],
 'no-regmove':['-O2','-frename-registers','-fno-regmove'],
 'no-gcse':['-O2','-frename-registers','-fno-gcse'],
 'no-reorder':['-O2','-frename-registers','-fno-reorder-blocks'],
 'no-ifconvert':['-O2','-frename-registers','-fno-if-conversion'],
 'no-ifconvert2':['-O2','-frename-registers','-fno-if-conversion2'],
 'no-peephole':['-O2','-frename-registers','-fno-peephole2'],
 'no-thread':['-O2','-frename-registers','-fno-thread-jumps'],
 'no-guess':['-O2','-frename-registers','-fno-guess-branch-probability'],
 'no-common':['-O2','-frename-registers','-fno-common'],
 'no-pic':['-O2','-frename-registers','-fno-pic'],
 'cost0':['-O2','-frename-registers','-mbranch-cost=0'],
 'cost1':['-O2','-frename-registers','-mbranch-cost=1'],
 'cost2':['-O2','-frename-registers','-mbranch-cost=2'],
 'cost3':['-O2','-frename-registers','-mbranch-cost=3'],
 'cost4':['-O2','-frename-registers','-mbranch-cost=4'],
 'tune386':['-O2','-frename-registers','-mcpu=i386'],
 'tune486':['-O2','-frename-registers','-mcpu=i486'],
 'tunepentium':['-O2','-frename-registers','-mcpu=pentium'],
 'tunepentiumpro':['-O2','-frename-registers','-mcpu=pentiumpro'],
 'tune686':['-O2','-frename-registers','-mcpu=i686'],
 'arch386':['-O2','-frename-registers','-march=i386'],
 'arch486':['-O2','-frename-registers','-march=i486'],
 'archpentium':['-O2','-frename-registers','-march=pentium'],
 'archpentiumpro':['-O2','-frename-registers','-march=pentiumpro'],
 'arch686':['-O2','-frename-registers','-march=i686'],
}
T=Path('/tmp/vuc-binutils215/mltemp-c2-target-reg-lifetime-oct15');T.mkdir(parents=True,exist_ok=True)
scores=[];sizes=collections.Counter();best=None
for (ctx,src),(fl,opts),snapshot in itertools.product(contexts.items(),flags.items(),('jan25','dec06')):
 key=ctx+'-'+fl+'-'+snapshot
 obj=T/(key+'.o')
 cp=subprocess.run(['/tmp/vuc-binutils215/g++-'+snapshot+'-as215',*opts,'-c',str(src),'-o',str(obj)],capture_output=True,text=True)
 if cp.returncode:scores.append(dict(key=key,error=cp.stderr[-200:]));continue
 try:res=m.run(obj,cfg)
 except Exception as e:scores.append(dict(key=key,error=str(e)));continue
 row=dict(key=key,length=res['length'],overlap=res['exact_bytes'],prefix=next((i for i in res['different_offsets']),res['length']),relocations=res['relocations'])
 scores.append(row);sizes[row['length']]+=1
 if row['length']==101 and row['relocations']==9 and (best is None or (row['overlap'],row['prefix'])>(best['overlap'],best['prefix'])):
  best=row;(T/'best.flags').write_text(' '.join(opts)+'\n');(T/'best.source').write_text(str(src)+'\n');(T/'best.o').write_bytes(obj.read_bytes())
(T/'results.json').write_text(json.dumps(scores,indent=2)+'\n')
print(json.dumps(dict(attempted=len(scores),sizes=dict(sizes),best_original_length=best,top=sorted((x for x in scores if 'overlap'in x),key=lambda r:(r['overlap'],r['prefix']),reverse=True)[:12]),indent=2))

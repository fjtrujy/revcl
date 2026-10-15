#!/usr/bin/env python3
"""VUC GetNextWord 23/135 C++03 scanner historical GCC-pass investigation.

Original data reference graph has 5 relocation *sites* to 3 unique globals.
Try genuine GCC3.3 optimization/CPU/branch controls to recover EAX position,
ECX input lifetime (current source inverts those register allocations).
Only exactly 135-byte candidates with all 5 resolved relocation sites
qualify as improvements.
"""
from pathlib import Path
import itertools,subprocess,json,importlib.util,collections
R=Path(__file__).resolve().parents[2]
s=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
cfg=R/'vuc/config/get_next_word_partial_oct15e_probe.json'
source=(R/'vuc/probes/GetNextWordByteBoundsPartialOct15e.cpp').read_text()
base=['-O2','-frename-registers','-falign-jumps=16']
controls={
 'baseline':[],
 'no-renaming':['-fno-rename-registers'],
 'j1':['-falign-jumps=1'],
 'j2':['-falign-jumps=2'],
 'j4':['-falign-jumps=4'],
 'j8':['-falign-jumps=8'],
 'j32':['-falign-jumps=32'],
 'l1':['-falign-labels=1'],
 'l2':['-falign-labels=2'],
 'l4':['-falign-labels=4'],
 'l8':['-falign-labels=8'],
 'loop1':['-falign-loops=1'],
 'loop2':['-falign-loops=2'],
 'loop4':['-falign-loops=4'],
 'loop8':['-falign-loops=8'],
 'no-regmove':['-fno-regmove'],
 'no-gcse':['-fno-gcse'],
 'no-crossjump':['-fno-crossjumping'],
 'no-reorder':['-fno-reorder-blocks'],
 'no-sched2':['-fno-schedule-insns2'],
 'no-sched1':['-fno-schedule-insns'],
 'no-ifconv':['-fno-if-conversion'],
 'no-ifconv2':['-fno-if-conversion2'],
 'no-thread':['-fno-thread-jumps'],
 'no-cprop':['-fno-cprop-registers'],
 'no-guess':['-fno-guess-branch-probability'],
 'no-peephole2':['-fno-peephole2'],
 'no-csefollow':['-fno-cse-follow-jumps'],
 'no-cseskip':['-fno-cse-skip-blocks'],
 'no-strict':['-fno-strict-aliasing'],
 'arch386':['-march=i386'],
 'arch486':['-march=i486'],
 'archpentium':['-march=pentium'],
 'archi686':['-march=i686'],
 'cpu386':['-mcpu=i386'],
 'cpu486':['-mcpu=i486'],
 'cpupentium':['-mcpu=pentium'],
 'cpui686':['-mcpu=i686'],
 'branch0':['-mbranch-cost=0'],
 'branch1':['-mbranch-cost=1'],
 'branch2':['-mbranch-cost=2'],
 'branch3':['-mbranch-cost=3'],
 'branch4':['-mbranch-cost=4'],
}
T=Path('/tmp/vuc-binutils215/get-next-word-partial-compiler-passes-oct15e');T.mkdir(exist_ok=True,parents=True)
rows=[];sizes=collections.Counter();best=None
for (name,extra),snapshot in itertools.product(controls.items(),('jan25','dec06')):
 tag=name+'-'+snapshot
 cpp=T/(tag+'.cpp');obj=T/(tag+'.o');cpp.write_text(source)
 flags=[*base,*extra]
 proc=subprocess.run(['/tmp/vuc-binutils215/g++-'+snapshot+'-as215',*flags,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if proc.returncode:rows.append(dict(tag=tag,error=proc.stderr[-130:]));continue
 try:d=m.run(obj,cfg)
 except ValueError as e:rows.append(dict(tag=tag,error=str(e)[:135]));continue
 rec=dict(tag=tag,length=d['length'],match=d['exact_bytes'],prefix=next(iter(d['different_offsets']),d['length']),relocations=d['relocations'])
 rows.append(rec);sizes[d['length']]+=1
 if d['length']==135 and d['relocations']==5 and (best is None or (rec['match'],rec['prefix'])>(best['match'],best['prefix'])):
  best=rec;(T/'best.cpp').write_text(source);(T/'best.o').write_bytes(obj.read_bytes());(T/'best.flags').write_text(' '.join(flags)+'\n')
  if rec['match']>23:print('IMPROVEMENT',json.dumps(rec),flush=True)
(T/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
print(json.dumps(dict(attempted=len(rows),sizes=dict(sizes),best=best,top=sorted((r for r in rows if 'match' in r),key=lambda r:(r['match'],r['prefix']),reverse=True)[:15]),indent=2))

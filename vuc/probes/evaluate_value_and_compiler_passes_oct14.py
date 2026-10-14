#!/usr/bin/env python3
"""Historical GCC optimization-pass controls around VUC Value && 190/325.

No source edits, register pinning, inline asm or padding. Candidate must
produce exact original 325-byte size and real TypeConvert relocation.
"""
from pathlib import Path
import itertools,importlib.util,json,subprocess,collections
R=Path(__file__).resolve().parents[2];sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=json.loads((R/'vuc/config/value_logical_and_partial_oct14_probe.json').read_text())
raw,elf=m.decomp.load();sec=next(s for s in elf['sections'] if s['name']=='.text')
addr=int(cfg['address'],0);original=raw[sec['offset']+addr-sec['address']:sec['offset']+addr-sec['address']+325]
base=['-O2','-frename-registers','-ffast-math','-falign-jumps=16','-falign-labels=1','-falign-loops=1']
opts={
 'baseline':[],
 'noguess':['-fno-guess-branch-probability'],
 'no-gcse':['-fno-gcse'],
 'no-cse-follow':['-fno-cse-follow-jumps'],
 'no-regmove':['-fno-regmove'],
 'no-reorder':['-fno-reorder-blocks'],
 'no-ifconv':['-fno-if-conversion'],
 'no-ifconv2':['-fno-if-conversion2'],
 'no-thread':['-fno-thread-jumps'],
 'no-crossjump':['-fno-crossjumping'],
 'no-sched2':['-fno-schedule-insns2'],
 'no-peephole2':['-fno-peephole2'],
 'no-inline':['-fno-inline'],
 'no-defer-pop':['-fno-defer-pop'],
 'no-omit-frame':['-fno-omit-frame-pointer'],
 'branch0':['-mbranch-cost=0'],
 'branch1':['-mbranch-cost=1'],
 'branch2':['-mbranch-cost=2'],
 'branch3':['-mbranch-cost=3'],
 'branch4':['-mbranch-cost=4'],
 'branch5':['-mbranch-cost=5'],
 'march-i386':['-march=i386'],
 'march-i486':['-march=i486'],
 'march-pentium':['-march=pentium'],
 'march-i686':['-march=i686'],
 'mtune-i386':['-mcpu=i386'],
 'mtune-i686':['-mcpu=i686'],
}
T=Path('/tmp/vuc-binutils215/value-and-historical-pass-controls');T.mkdir(parents=True,exist_ok=True)
rows=[];best=None;counts=collections.Counter()
for key,extra in opts.items():
 flags=base+extra;obj=T/(key+'.o')
 cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*flags,'-c',str(R/cfg['source']),'-o',str(obj)],capture_output=True,text=True)
 if cp.returncode:rows.append(dict(key=key,error=cp.stderr[-180:]));continue
 try:body,rel=m.prepare_named_function(obj.read_bytes(),cfg['function'],cfg['relocations'],addr)
 except ValueError as e:rows.append(dict(key=key,error=str(e)));continue
 score=sum(a==b for a,b in zip(body,original))
 prefix=next((i for i,(a,b) in enumerate(zip(body,original)) if a!=b),min(len(body),len(original)))
 row=dict(key=key,length=len(body),relocations=rel,overlap=score,prefix=prefix)
 rows.append(row);counts[len(body)]+=1
 if len(body)==325 and rel==1 and (best is None or (score,prefix)>(best['overlap'],best['prefix'])):
  best=row;(T/'best.o').write_bytes(obj.read_bytes());(T/'best.flags').write_text(' '.join(flags)+'\n')
(T/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
print(json.dumps(dict(attempted=len(rows),sizes=dict(counts),best=best,top=sorted((r for r in rows if 'overlap' in r),key=lambda r:(r['overlap'],r['prefix']),reverse=True)[:10]),indent=2))

#!/usr/bin/env python3
"""Period GCC 3.3.3 compiler-pass controls for VUC 235-byte Value division.
A genuine 3,1,2,0 x87 lane order emitted 237 bytes and overlapped 212 of
235 originals; seek real source/optimizer choices making true 235 bytes.
"""
from pathlib import Path
import importlib.util,itertools,json,subprocess,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=json.loads((R/'vuc/config/value_division_partial_oct13.json').read_text())
raw,elf=m.decomp.load();ss=next(s for s in elf['sections'] if s['name']=='.text');addr=int(cfg['address'],0)
ref=raw[ss['offset']+addr-ss['address']:ss['offset']+addr-ss['address']+235]
src=(R/cfg['source']).read_text()
old='\n'.join(f'            result.values[{i}] *= 1.0f / temporary.values[{i}];' for i in range(4))
assert old in src
forms={
 'saved':'case 3: { float divisor = temporary.values[0]; result.values[0] /= divisor; break; }',
 'direct':'case 3: result.values[0] /= temporary.values[0]; break;',
 'bytemp':'case 3: { const float d=temporary.values[0]; result.values[0] /= d; break; }'}
opts={
 'baseline':[],
 'no-sched1':['-fno-schedule-insns'],
 'no-sched2':['-fno-schedule-insns2'],
 'no-regmove':['-fno-regmove'],
 'no-crossjump':['-fno-crossjumping'],
 'no-gcse':['-fno-gcse'],
 'no-reorder':['-fno-reorder-blocks'],
 'no-peephole2':['-fno-peephole2'],
 'no-csefollow':['-fno-cse-follow-jumps'],
 'no-cseskip':['-fno-cse-skip-blocks'],
 'no-ifconversion':['-fno-if-conversion'],
 'no-ifconversion2':['-fno-if-conversion2'],
 'no-thread':['-fno-thread-jumps'],
 'no-guess':['-fno-guess-branch-probability'],
 'no-strict':['-fno-strict-aliasing'],
 'no-builtin':['-fno-builtin'],
 'no-omitframe':['-fno-omit-frame-pointer'],
 'no-inline':['-fno-inline'],
 'no-cprop':['-fno-cprop-registers'],
 'no-web':['-fno-web'],
 'cost0':['-mbranch-cost=0'],
 'cost1':['-mbranch-cost=1'],
 'cost2':['-mbranch-cost=2'],
 'cost3':['-mbranch-cost=3'],
 'cost4':['-mbranch-cost=4'],
 'align-labels1':['-falign-labels=1'],
 'align-labels2':['-falign-labels=2'],
 'align-labels4':['-falign-labels=4'],
 'align-loops1':['-falign-loops=1'],
 'align-loops4':['-falign-loops=4'],
 'arch386':['-march=i386'],
 'arch486':['-march=i486'],
 'archpentium':['-march=pentium'],
 'cpu386':['-mcpu=i386'],
 'cpu486':['-mcpu=i486'],
 'cpupentium':['-mcpu=pentium'],
 'cpu686':['-mcpu=i686'],
}
T=Path('/tmp/vuc-binutils215/vuc-div-pass-order-oct15b');T.mkdir(parents=True,exist_ok=True)
ranks=[];sizes=collections.Counter();best=None
for lane,form,(name,extra) in itertools.product(((3,1,2,0),(0,1,2,3),(3,2,1,0),(0,1,3,2)),forms,opts.items()):
 tag=f'{"".join(map(str,lane))}-{form}-{name}'
 text=src.replace(old,'\n'.join(f'            result.values[{i}] *= 1.0f / temporary.values[{i}];' for i in lane)).replace(forms['saved'],forms[form])
 cpp=T/(tag+'.cpp');obj=T/(tag+'.o');cpp.write_text(text)
 flags=['-O2','-frename-registers','-falign-jumps=1',*extra]
 proc=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*flags,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if proc.returncode:ranks.append(dict(tag=tag,error=proc.stderr[-110:]));continue
 try:b,r=m.prepare_named_function(obj.read_bytes(),cfg['function'],cfg['relocations'],addr)
 except ValueError as e:ranks.append(dict(tag=tag,error=str(e)));continue
 score=sum(x==y for x,y in zip(ref,b));pfx=next((i for i,(x,y) in enumerate(zip(ref,b)) if x!=y),min(len(ref),len(b)))
 rec=dict(tag=tag,length=len(b),relocations=r,match=score,prefix=pfx);ranks.append(rec);sizes[len(b)]+=1
 if len(b)==235 and r==1 and (best is None or (score,pfx)>(best['match'],best['prefix'])):
  best=rec;(T/'best.cpp').write_text(text);(T/'best.flags').write_text(' '.join(flags)+'\n');(T/'best.o').write_bytes(obj.read_bytes())
(T/'results.json').write_text(json.dumps(ranks,indent=2)+'\n')
print(json.dumps(dict(attempted=len(ranks),sizes=dict(sizes),best=best,top=sorted((x for x in ranks if 'match' in x),key=lambda r:(r['match'],r['prefix']),reverse=True)[:12]),indent=2))

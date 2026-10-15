#!/usr/bin/env python3
"""VUC Value::Set: 213/459 genuine ELF overlap with complete original 15 refs.
Historical GCC pass controls attempt to restore original 455-byte length
without handcoded x86, synthetic reference targets or dummy padding.
"""
from pathlib import Path
import json,importlib.util,itertools,subprocess,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
c=json.loads((R/'vuc/config/value_set_constval_partial_oct15b.json').read_text())
raw,elf=m.decomp.load();s=next(x for x in elf['sections'] if x['name']=='.text');a=int(c['address'],0);ref=raw[s['offset']+a-s['address']:s['offset']+a-s['address']+455]
src=(R/'vuc/probes/ValueConstValSetPartialOct15b.cpp').read_text().replace('#include "../src/MLGameString.h"','#include "'+str(R/'vuc/src/MLGameString.h')+'"').replace('char scratch[100]','char scratch[64]')
src=src.replace('        const char *second = expression->source.c_str();\n','').replace('std::strcmp(second, "false")','std::strcmp(expression->source.c_str(), "false")')
src=src.replace('            std::memcpy(scratch, text, length);','            const char *next = comma + 1;\n            std::memcpy(scratch, text, length);').replace('            text = comma + 1;','            text = next;')
controls={
 'base':[],
 'no-thread':['-fno-thread-jumps'],
 'no-csefollow':['-fno-cse-follow-jumps'],
 'no-cseskip':['-fno-cse-skip-blocks'],
 'no-gcse':['-fno-gcse'],
 'no-regmove':['-fno-regmove'],
 'no-peephole2':['-fno-peephole2'],
 'no-sched1':['-fno-schedule-insns'],
 'no-sched2':['-fno-schedule-insns2'],
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
 'cpu386':['-mcpu=i386'],
 'cpu486':['-mcpu=i486'],
 'cpu586':['-mcpu=pentium'],
 'cpu686':['-mcpu=i686'],
 'arch386':['-march=i386'],
 'arch486':['-march=i486'],
 'arch586':['-march=pentium'],
 'arch686':['-march=i686'],
 'alignloops1':['-falign-loops=1'],
 'alignloops2':['-falign-loops=2'],
 'alignloops4':['-falign-loops=4'],
 'labels1':['-falign-labels=1'],
 'labels2':['-falign-labels=2'],
 'labels4':['-falign-labels=4'],
 'jump1':['-falign-jumps=1'],
 'jump2':['-falign-jumps=2'],
 'jump3':['-falign-jumps=3'],
 'jump5':['-falign-jumps=5'],
 'jump8':['-falign-jumps=8'],
 'no-renaming':['-fno-rename-registers'],
 'opsize':['-Os'],
 'op3':['-O3'],
 'op1':['-O1'],
}
T=Path('/tmp/vuc-binutils215/value-set-213-compiler-controls-oct15c');T.mkdir(exist_ok=True,parents=True)
ranks=[];sizes=collections.Counter();best=None
for (name,opts),snapshot in itertools.product(controls.items(),('jan25','dec06')):
 tag=f'{name}-{snapshot}'
 cpp=T/(tag+'.cpp');obj=T/(tag+'.o');cpp.write_text(src)
 flags=['-O2','-frename-registers','-falign-jumps=4','-fno-crossjumping',*opts]
 res=subprocess.run(['/tmp/vuc-binutils215/g++-'+snapshot+'-as215',*flags,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if res.returncode:ranks.append(dict(tag=tag,error=res.stderr[-130:]));continue
 try:b,r=m.prepare_named_function(obj.read_bytes(),c['function'],c['relocations'],a,literal_ranges=c['relocation_literals'])
 except ValueError as e:ranks.append(dict(tag=tag,error=str(e)));continue
 match=sum(x==y for x,y in zip(ref,b));prefix=next((i for i,(x,y) in enumerate(zip(ref,b)) if x!=y),min(len(ref),len(b)))
 row=dict(tag=tag,length=len(b),relocations=r,match=match,prefix=prefix)
 ranks.append(row);sizes[len(b)]+=1
 if len(b)==455 and r==15 and (best is None or (match,prefix)>(best['match'],best['prefix'])):
  best=row;(T/'best.cpp').write_text(src);(T/'best.o').write_bytes(obj.read_bytes());(T/'best.flags').write_text(' '.join(flags)+'\n')
(T/'results.json').write_text(json.dumps(ranks,indent=2)+'\n')
print(json.dumps(dict(attempted=len(ranks),sizes=dict(sizes),best=best,top=sorted((r for r in ranks if 'match' in r),key=lambda r:(r['match'],r['prefix']),reverse=True)[:15]),indent=2))

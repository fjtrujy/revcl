#!/usr/bin/env python3
"""GCC 3.3.3 non-semantic real switch-likelihood layout controls for VUC Value::Set.
Test original 455-byte + 15-reference parser after source-level 64-byte
scratch buffer and preserved independent boolean parsing path.
"""
from pathlib import Path
import json,subprocess,itertools,importlib.util,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=R/'vuc/config/value_set_constval_partial_oct15b.json'
source=(R/'vuc/probes/ValueConstValSetPartialOct15b.cpp').read_text().replace('#include "../src/MLGameString.h"','#include "'+str(R/'vuc/src/MLGameString.h')+'"').replace('char scratch[100]','char scratch[64]')
flags={
 'j1':['-O2','-frename-registers','-falign-jumps=1','-fno-crossjumping'],
 'j4':['-O2','-frename-registers','-falign-jumps=4','-fno-crossjumping'],
 'j4l2':['-O2','-frename-registers','-falign-jumps=4','-falign-labels=2','-fno-crossjumping'],
 'saved':['-O2','-frename-registers','-falign-jumps=4','-falign-labels=2','-fno-crossjumping','-fno-reorder-blocks'],
 'plain':['-O2','-falign-jumps=4','-fno-crossjumping'],
}
T=Path('/tmp/vuc-binutils215/value-set-type-likelihood-oct15c');T.mkdir(parents=True,exist_ok=True)
scores=[];sizes=collections.Counter();best=None
for expected,(mode,opts) in itertools.product((None,0,1,2,3,4,-1),flags.items()):
 tag=f'{"plain" if expected is None else "expect"+str(expected)}-{mode}'
 code=source.replace('    switch (type) {','    switch (type) {' if expected is None else f'    switch (__builtin_expect(type, {expected})) {{')
 cpp=T/(tag+'.cpp');obj=T/(tag+'.o');cpp.write_text(code)
 p=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if p.returncode:scores.append(dict(tag=tag,error=p.stderr[-160:]));continue
 symbol=next((line for line in subprocess.check_output(['nm','-S',str(obj)],text=True).splitlines() if line.endswith('_ZN5Value3SetEPK15ASTConstValExpr')),None)
 n=int(symbol.split()[1],16) if symbol else -1;sizes[n]+=1
 if n!=455:scores.append(dict(tag=tag,length=n));continue
 try:d=m.run(obj,cfg)
 except ValueError as e:scores.append(dict(tag=tag,length=n,error=str(e)[:150]));continue
 row=dict(tag=tag,length=n,match=d['exact_bytes'],relocations=d['relocations'],literals=d['verified_literal_bytes'],prefix=next(iter(d['different_offsets']),n))
 scores.append(row)
 if best is None or (row['match'],row['prefix'])>(best['match'],best['prefix']):
  best=row;(T/'best.cpp').write_text(code);(T/'best.o').write_bytes(obj.read_bytes());(T/'best.flags').write_text(' '.join(opts)+'\n')
(T/'results.json').write_text(json.dumps(scores,indent=2)+'\n')
print(json.dumps(dict(attempted=len(scores),sizes=dict(sizes),best=best),indent=2))

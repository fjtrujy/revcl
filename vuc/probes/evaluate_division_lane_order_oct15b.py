#!/usr/bin/env python3
"""VUC 2004.1 genuine vector division lane-order C++03 experiment.
Transfer the successful Value+ lane ordering idea to its arithmetic sibling.
Preserve original 235-byte function size and authentic TypeConvert relocation.
"""
from pathlib import Path
import importlib.util,itertools,json,subprocess,collections
R=Path(__file__).resolve().parents[2]
s=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
cfg=json.loads((R/'vuc/config/value_division_partial_oct13.json').read_text());raw,elf=m.decomp.load()
sec=next(x for x in elf['sections'] if x['name']=='.text');a=int(cfg['address'],0)
ref=raw[sec['offset']+a-sec['address']:sec['offset']+a-sec['address']+cfg['size']]
base=(R/cfg['source']).read_text()
start=base.index('Value Value::operator/(const Value &other) const')
fragment=base[start:]
old='\n'.join(f'            result.values[{i}] *= 1.0f / temporary.values[{i}];' for i in range(4))
assert old in fragment
scalar='case 3: { float divisor = temporary.values[0]; result.values[0] /= divisor; break; }'
assert scalar in fragment
sforms={
 'saved':scalar,
 'direct':'case 3: result.values[0] /= temporary.values[0]; break;',
 'assignment':'case 3: result.values[0] = result.values[0] / temporary.values[0]; break;',
 'temp':'case 3: { const float d=temporary.values[0]; result.values[0] /= d; break; }',
}
flags={
 'j1':['-O2','-frename-registers','-falign-jumps=1'],
 'j4':['-O2','-frename-registers','-falign-jumps=4'],
 'j8':['-O2','-frename-registers','-falign-jumps=8'],
 'j16':['-O2','-frename-registers','-falign-jumps=16'],
 'j8l1':['-O2','-frename-registers','-falign-jumps=8','-falign-labels=1']
}
T=Path('/tmp/vuc-binutils215/vuc-div-lanes-oct15b');T.mkdir(parents=True,exist_ok=True)
rows=[];sizes=collections.Counter();best=None
for lanes,(form,text),(flag,opts) in itertools.product(itertools.permutations(range(4)),sforms.items(),flags.items()):
 tag=''.join(map(str,lanes))+'-'+form+'-'+flag
 arr='\n'.join(f'            result.values[{i}] *= 1.0f / temporary.values[{i}];' for i in lanes)
 candidate=base.replace(old,arr).replace(scalar,text)
 cpp=T/(tag+'.cpp');obj=T/(tag+'.o');cpp.write_text(candidate)
 cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if cp.returncode:rows.append(dict(tag=tag,error=cp.stderr[-120:]));continue
 try:body,r=m.prepare_named_function(obj.read_bytes(),cfg['function'],cfg['relocations'],a)
 except ValueError as e:rows.append(dict(tag=tag,error=str(e)));continue
 score=sum(x==y for x,y in zip(ref,body))
 pfx=next((i for i,(x,y) in enumerate(zip(ref,body)) if x!=y),min(len(ref),len(body)))
 row=dict(tag=tag,length=len(body),relocations=r,match=score,prefix=pfx)
 rows.append(row);sizes[len(body)]+=1
 if len(body)==cfg['size'] and r==1 and (best is None or (score,pfx)>(best['match'],best['prefix'])):
  best=row;(T/'best.cpp').write_text(candidate);(T/'best.o').write_bytes(obj.read_bytes());(T/'best.flags').write_text(' '.join(opts)+'\n')
(T/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
print(json.dumps(dict(attempted=len(rows),sizes=dict(sizes),best=best,top=sorted((r for r in rows if 'match' in r),key=lambda r:(r['match'],r['prefix']),reverse=True)[:12]),indent=2))

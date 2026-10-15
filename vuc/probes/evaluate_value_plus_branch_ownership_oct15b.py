#!/usr/bin/env python3
"""VUC original Value+ branch-owner study using 2004-era C++03 predicates.
The 216-byte natural 0,1,2,3 source has near-perfect bytes but lacks the
3 genuine branch-alignment bytes before the original scalar block at +0xd0.
Test semantic switch-vs-if ownership rather than padding or assembler.
"""
from pathlib import Path
import importlib.util,itertools,subprocess,json,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=json.loads((R/'vuc/config/value_addition_partial_oct14_probe.json').read_text())
raw,elf=m.decomp.load();s=next(x for x in elf['sections'] if x['name']=='.text');addr=int(cfg['address'],0)
ref=raw[s['offset']+addr-s['address']:s['offset']+addr-s['address']+219]
src=(R/'vuc/src/ValueArithmeticUnit.cpp').read_text()
start=src.index('Value Value::operator+(const Value &other) const');end=src.index('Value Value::operator-(',start)
base=src[start:end]
a=base.index('        switch (result.type) {')
b=base.index('        }\n    }\n    return result;',a)+len('        }')
intro='''        switch (result.type) {
        case 1: result.bits += temporary.bits; break;
        case 3: result.values[0] += temporary.values[0]; break;
        case 4:
            result.values[0] += temporary.values[0];
            result.values[1] += temporary.values[1];
            result.values[2] += temporary.values[2];
            result.values[3] += temporary.values[3];
            break;
        }'''
assert intro==base[a:b]
v=lambda order:'\n'.join(f'            result.values[{i}] += temporary.values[{i}];' for i in order)
orders=[(0,1,2,3),(3,1,2,0),(0,2,1,3),(1,0,2,3)]
pred={'plain':lambda typ:f'result.type == {typ}',
      'unlikely':lambda typ:f'__builtin_expect(result.type == {typ}, 0)',
      'likely':lambda typ:f'__builtin_expect(result.type == {typ}, 1)'}
def builder(mode,order,hint):
 vblock=v(order)
 t={k:pred[hint](k) for k in (1,3,4)}
 scalar='result.values[0] += temporary.values[0];'
 integer='result.bits += temporary.bits;'
 vector='{\n'+vblock+'\n        }'
 if mode=='switch':return intro.replace(v((0,1,2,3)),vblock)
 if mode=='scalar-first':return f'        if ({t[3]}) {{ {scalar} }} else switch (result.type) {{\n        case 1: {integer} break;\n        case 4: {vblock}\n        break;\n        }}'
 if mode=='scalar-last':return f'        if ({t[3]}) {{ {scalar} }} else if ({t[4]}) {vector} else if ({t[1]}) {{ {integer} }}'
 if mode=='vector-first':return f'        if ({t[4]}) {vector} else switch (result.type) {{\n        case 1: {integer} break;\n        case 3: {scalar} break;\n        }}'
 if mode=='vector-last':return f'        if ({t[4]}) {vector} else if ({t[3]}) {{ {scalar} }} else if ({t[1]}) {{ {integer} }}'
 if mode=='integer-first':return f'        if ({t[1]}) {{ {integer} }} else switch (result.type) {{\n        case 3: {scalar} break;\n        case 4: {vblock}\n        break;\n        }}'
 if mode=='integer-last':return f'        if ({t[1]}) {{ {integer} }} else if ({t[3]}) {{ {scalar} }} else if ({t[4]}) {vector}'
 if mode=='scalar-nested':return f'        if (!({t[3]})) {{ if ({t[1]}) {{ {integer} }} else if ({t[4]}) {vector} }} else {{ {scalar} }}'
 if mode=='vector-nested':return f'        if (!({t[4]})) {{ if ({t[1]}) {{ {integer} }} else if ({t[3]}) {{ {scalar} }} }} else {vector}'
 raise ValueError(mode)
flags={'j1':['-O2','-frename-registers','-falign-jumps=1'],
       'j2':['-O2','-frename-registers','-falign-jumps=2'],
       'j4':['-O2','-frename-registers','-falign-jumps=4'],
       'j1l4':['-O2','-frename-registers','-falign-jumps=1','-falign-labels=4'],
       'j4l1':['-O2','-frename-registers','-falign-jumps=4','-falign-labels=1']}
T=Path('/tmp/vuc-binutils215/vuc-plus-branch-owner-oct15b');T.mkdir(parents=True,exist_ok=True)
rows=[];best=None;lengths=collections.Counter()
for mode,order,hint,(fl,opts) in itertools.product(('switch','scalar-first','scalar-last','vector-first','vector-last','integer-first','integer-last','scalar-nested','vector-nested'),orders,pred,flags.items()):
 tag=f'{mode}-{"".join(map(str,order))}-{hint}-{fl}'
 form=builder(mode,order,hint)
 out=src[:start]+base[:a]+form+base[b:]+src[end:]
 cpp=T/(tag+'.cpp');obj=T/(tag+'.o');cpp.write_text(out)
 proc=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if proc.returncode:rows.append(dict(tag=tag,error=proc.stderr[-190:]));continue
 try:bdy,r=m.prepare_named_function(obj.read_bytes(),cfg['function'],cfg['relocations'],addr)
 except ValueError as e:rows.append(dict(tag=tag,error=str(e)));continue
 score=sum(x==y for x,y in zip(ref,bdy));pfx=next((i for i,(x,y) in enumerate(zip(ref,bdy)) if x!=y),min(len(ref),len(bdy)))
 record=dict(tag=tag,length=len(bdy),relocations=r,match=score,prefix=pfx);rows.append(record);lengths[len(bdy)]+=1
 if len(bdy)==219 and r==1 and (best is None or (score,pfx)>(best['match'],best['prefix'])):
  best=record;(T/'best.cpp').write_text(out);(T/'best.flags').write_text(' '.join(opts)+'\n');(T/'best.o').write_bytes(obj.read_bytes())
(T/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
print(json.dumps(dict(attempted=len(rows),sizes=dict(lengths),best=best,top=sorted((r for r in rows if 'match' in r),key=lambda r:(r['match'],r['prefix']),reverse=True)[:18]),indent=2))

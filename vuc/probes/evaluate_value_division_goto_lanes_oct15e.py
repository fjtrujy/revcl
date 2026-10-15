#!/usr/bin/env python3
"""VUC Value::operator/ C++03 natural negative dispatch / goto return owners.

Prior 3120-lane source emits 237 bytes and overlaps 212 original bytes,
but original is strictly 235; the best accepted 235-byte source is 178/235.
Try the successful negative-guard/goto method from IsAccumulatorable
without changing authentic floating-point operations, TypeConvert call
or putting byte fillers in source.
"""
from pathlib import Path
import json,itertools,subprocess,importlib.util,collections
R=Path(__file__).resolve().parents[2]
s=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
cfg=json.loads((R/'vuc/config/value_division_partial_oct13.json').read_text())
src=(R/cfg['source']).read_text();a=src.index('Value Value::operator/(const Value &other) const')
prefix=src[:a]
intexpr='result.bits = (int)result.bits / (int)temporary.bits;'
scalar='float divisor = temporary.values[0]; result.values[0] /= divisor;'
patterns={
 'switch-break':'''switch (result.type) {
        case 1: INT break;
        case 3: { SCALAR break; }
        case 4: VECTOR break;
        }''',
 'switch-goto-done':'''switch (result.type) {
        case 1: INT goto done;
        case 3: { SCALAR goto done; }
        case 4: VECTOR goto done;
        }''',
 'switch-vector-goto':'''switch (result.type) {
        case 1: INT break;
        case 3: { SCALAR break; }
        case 4: goto vector;
        }
        goto done;
vector: VECTOR goto done;''',
 'switch-scalar-goto':'''switch (result.type) {
        case 1: INT break;
        case 3: goto scalar;
        case 4: VECTOR goto done;
        }
        goto done;
scalar: { SCALAR goto done; }''',
 'switch-int-goto':'''switch (result.type) {
        case 1: goto integer;
        case 3: { SCALAR break; }
        case 4: VECTOR goto done;
        }
        goto done;
integer: INT goto done;''',
 'switch-three-goto':'''switch (result.type) {
        case 1: goto integer;
        case 3: goto scalar;
        case 4: goto vector;
        }
        goto done;
integer: INT goto done;
scalar: { SCALAR goto done; }
vector: VECTOR goto done;''',
 'switch-inverted-order':'''switch (result.type) {
        case 4: VECTOR goto done;
        case 3: { SCALAR goto done; }
        case 1: INT goto done;
        }''',
 'if-integer-first':'''if(result.type==1){ INT goto done; }
        if(result.type==3){ SCALAR goto done; }
        if(result.type==4){ VECTOR goto done; }''',
 'if-vector-first':'''if(result.type==4){ VECTOR goto done; }
        if(result.type==3){ SCALAR goto done; }
        if(result.type==1){ INT goto done; }''',
 'if-scalar-first':'''if(result.type==3){ SCALAR goto done; }
        if(result.type==1){ INT goto done; }
        if(result.type==4){ VECTOR goto done; }''',
 'negative-scalar':'''if(result.type!=3)goto notscalar;
        SCALAR goto done;
notscalar: if(result.type==1){INT goto done;}
        if(result.type==4){VECTOR goto done;}''',
 'negative-vector':'''if(result.type!=4)goto notvector;
        VECTOR goto done;
notvector: if(result.type==1){INT goto done;}
        if(result.type==3){SCALAR goto done;}''',
 'negative-integer':'''if(result.type!=1)goto notinteger;
        INT goto done;
notinteger: if(result.type==3){SCALAR goto done;}
        if(result.type==4){VECTOR goto done;}''',
 'if-inverted-scalar':'''if(result.type!=3){if(result.type==1){INT goto done;} if(result.type==4){VECTOR goto done;}}
        else{SCALAR goto done;}''',
 'if-inverted-vector':'''if(result.type!=4){if(result.type==1){INT goto done;} if(result.type==3){SCALAR goto done;}}
        else{VECTOR goto done;}''',
 'switch-case3-last':'''switch(result.type){
        case 1: INT break;
        case 4: VECTOR goto done;
        case 3: {SCALAR goto done;}
        }''',
}
gates={
 'outer-if':'if(TypeConvert(*this,other,result,temporary)) { BLOCK }',
 'early-negative':'if(!TypeConvert(*this,other,result,temporary))goto done; BLOCK',
 'outer-positive':'if(TypeConvert(*this,other,result,temporary))goto work;goto done;work:BLOCK'
}
flags={
 'j1':['-O2','-frename-registers','-falign-jumps=1'],
 'j2':['-O2','-frename-registers','-falign-jumps=2'],
 'j4':['-O2','-frename-registers','-falign-jumps=4'],
 'j8':['-O2','-frename-registers','-falign-jumps=8'],
 'j16':['-O2','-frename-registers','-falign-jumps=16'],
 'j1nocross':['-O2','-frename-registers','-falign-jumps=1','-fno-crossjumping'],
 'j8noreorder':['-O2','-frename-registers','-falign-jumps=8','-fno-reorder-blocks'],
}
T=Path('/tmp/vuc-binutils215/value-div-goto-lanes-oct15e');T.mkdir(exist_ok=True,parents=True)
raw,elf=m.decomp.load();sect=next(x for x in elf['sections'] if x['name']=='.text');va=int(cfg['address'],0);ref=raw[sect['offset']+va-sect['address']:sect['offset']+va-sect['address']+235]
rows=[];sizes=collections.Counter();best=None
for order,(pn,shape),(gn,gate),(fl,opts) in itertools.product(((3,1,2,0),(0,1,2,3),(3,2,1,0)),patterns.items(),gates.items(),flags.items()):
 vec='\n'.join(f'            result.values[{i}] *= 1.0f / temporary.values[{i}];' for i in order)
 dispatch=shape.replace('INT',intexpr).replace('SCALAR',scalar).replace('VECTOR',vec)
 actual='''Value Value::operator/(const Value &other) const
{
    Value result;
    Value temporary;
    GATE
done:
    return result;
}
'''.replace('GATE',gate.replace('BLOCK',dispatch))
 tag=''.join(map(str,order))+'-'+pn+'-'+gn+'-'+fl
 cpp=T/(tag+'.cpp');obj=T/(tag+'.o');cpp.write_text(prefix+actual)
 p=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if p.returncode:rows.append(dict(tag=tag,error=p.stderr[-120:]));continue
 try:body,rel=m.prepare_named_function(obj.read_bytes(),cfg['function'],cfg['relocations'],va)
 except ValueError as e:rows.append(dict(tag=tag,error=str(e)));continue
 score=sum(x==y for x,y in zip(ref,body));px=next((i for i,(x,y) in enumerate(zip(ref,body)) if x!=y),min(len(ref),len(body)))
 row=dict(tag=tag,length=len(body),match=score,prefix=px,relocations=rel);rows.append(row);sizes[len(body)]+=1
 if len(body)==235 and rel==1 and (best is None or (score,px)>(best['match'],best['prefix'])):
  best=row;(T/'best.cpp').write_text(cpp.read_text());(T/'best.flags').write_text(' '.join(opts)+'\n');(T/'best.o').write_bytes(obj.read_bytes())
  if score>178:print('IMPROVE',json.dumps(row),flush=True)
  if score==235:print('EXACT',json.dumps(row),flush=True);break
(T/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
print(json.dumps(dict(attempted=len(rows),sizes=dict(sizes),best=best,top=sorted((r for r in rows if 'match' in r),key=lambda x:(x['match'],x['prefix']),reverse=True)[:12]),indent=2))

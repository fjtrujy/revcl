#!/usr/bin/env python3
"""Historical GCC3.3.3 authentic control-flow ownership of VUC Value::operator+.
Goal: preserve original vector lane arithmetic 0/1/2/3, while recovering
the 219-byte original length and compiler-owned 3-byte scalar-block
alignment. No asm/no padding/no register forcing/fake relocations.
"""
from pathlib import Path
import importlib.util,itertools,json,subprocess,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=json.loads((R/'vuc/config/value_addition_partial_oct14_probe.json').read_text())
base=(R/'vuc/probes/ValueAdditionLaneOrderOct15.cpp').read_text()
start=base.index('Value Value::operator+(const Value &other) const')
end=base.index('Value Value::operator-(const Value &other) const',start)
function=base[start:end]
vector='\n'.join(f'            result.values[{i}] += temporary.values[{i}];' for i in range(4))
init='''Value Value::operator+(const Value &other) const
{
    Value result;
    Value temporary;
    if (TypeConvert(*this, other, result, temporary)) {
$BODY
    }
$END
}
'''
frags={
 'int':'result.bits += temporary.bits;',
 'scalar':'result.values[0] += temporary.values[0];',
 'vec':vector
}
forms={}
forms['switch-break']='''        switch (result.type) {
        case 1: $INT break;
        case 3: $SCALAR break;
        case 4:
$VEC
            break;
        }'''
forms['switch-goto-tail']='''        switch (result.type) {
        case 1: $INT goto done;
        case 3: $SCALAR goto done;
        case 4:
$VEC
            goto done;
        }'''
forms['switch-goto-cases']='''        switch (result.type) {
        case 1: goto integer;
        case 3: goto scalar;
        case 4: goto vector;
        }
        goto done;
integer: $INT goto done;
scalar: $SCALAR goto done;
vector:
$VEC
        goto done;'''
forms['switch-goto-mix']='''        switch (result.type) {
        case 1: goto integer;
        case 3: goto scalar;
        case 4:
$VEC
            goto done;
        }
        goto done;
integer: $INT goto done;
scalar: $SCALAR goto done;'''
forms['switch-goto-scalar']='''        switch (result.type) {
        case 1: $INT break;
        case 3: goto scalar;
        case 4:
$VEC
            goto done;
        }
        goto done;
scalar: $SCALAR goto done;'''
forms['switch-goto-vector']='''        switch (result.type) {
        case 1: $INT break;
        case 3: $SCALAR goto done;
        case 4: goto vector;
        }
        goto done;
vector:
$VEC
        goto done;'''
forms['switch-goto-int']='''        switch (result.type) {
        case 1: goto integer;
        case 3: $SCALAR break;
        case 4:
$VEC
            break;
        }
        goto done;
integer: $INT goto done;'''
forms['if-cases']='''        if (result.type == 1) { $INT }
        else if (result.type == 3) { $SCALAR }
        else if (result.type == 4) {
$VEC
        }'''
forms['if-cases-reverse']='''        if (result.type == 4) {
$VEC
        } else if (result.type == 3) { $SCALAR }
        else if (result.type == 1) { $INT }'''
forms['if-goto-tail']='''        if (result.type == 1) { $INT goto done; }
        if (result.type == 3) { $SCALAR goto done; }
        if (result.type == 4) {
$VEC
            goto done;
        }'''
forms['if-goto-reverse']='''        if (result.type == 4) {
$VEC
            goto done;
        }
        if (result.type == 3) { $SCALAR goto done; }
        if (result.type == 1) { $INT goto done; }'''
forms['if-goto-labelled']='''        if (result.type == 3) goto scalar;
        if (result.type == 4) goto vector;
        if (result.type == 1) goto integer;
        goto done;
integer: $INT goto done;
scalar: $SCALAR goto done;
vector:
$VEC
        goto done;'''
forms['if-goto-vectorfirst']='''        if (result.type == 4) goto vector;
        if (result.type == 3) goto scalar;
        if (result.type == 1) goto integer;
        goto done;
vector:
$VEC
        goto done;
scalar: $SCALAR goto done;
integer: $INT goto done;'''
forms['if-unlikely']='''        if (__builtin_expect(result.type == 3, 0)) { $SCALAR goto done; }
        if (result.type == 1) { $INT goto done; }
        if (result.type == 4) {
$VEC
            goto done;
        }'''
forms['if-likely']='''        if (__builtin_expect(result.type == 3, 1)) { $SCALAR goto done; }
        if (result.type == 1) { $INT goto done; }
        if (result.type == 4) {
$VEC
            goto done;
        }'''
forms['switch-break-to-return']='''        switch (result.type) {
        case 1: $INT break;
        case 3: $SCALAR break;
        case 4:
$VEC
            break;
        }
        goto done;'''
forms['vector-invert-guard']='''        if (result.type != 3) {
            if (result.type == 1) { $INT goto done; }
            if (result.type == 4) {
$VEC
               goto done;
            }
        } else { $SCALAR goto done; }'''
forms['goto-negative']='''        if (result.type != 3) goto notscalar;
        $SCALAR
        goto done;
notscalar:
        if (result.type == 1) { $INT goto done; }
        if (result.type == 4) {
$VEC
            goto done;
        }'''
forms['goto-negative-vector']='''        if (result.type != 4) goto notvector;
$VEC
        goto done;
notvector:
        if (result.type == 3) { $SCALAR goto done; }
        if (result.type == 1) { $INT goto done; }'''
forms['goto-negative-int']='''        if (result.type != 1) goto notint;
        $INT
        goto done;
notint:
        if (result.type == 3) { $SCALAR goto done; }
        if (result.type == 4) {
$VEC
            goto done;
        }'''
forms['switch-literal-ordered']='''        switch (result.type) {
        case 4:
$VEC
            goto done;
        case 3: $SCALAR goto done;
        case 1: $INT goto done;
        }'''
forms['switch-literal-swap']='''        switch (result.type) {
        case 3: $SCALAR goto done;
        case 1: $INT goto done;
        case 4:
$VEC
            goto done;
        }'''
flags={
 'j1':['-O2','-frename-registers','-falign-jumps=1'],
 'j2':['-O2','-frename-registers','-falign-jumps=2'],
 'j4':['-O2','-frename-registers','-falign-jumps=4'],
 'j8':['-O2','-frename-registers','-falign-jumps=8'],
 'j16':['-O2','-frename-registers','-falign-jumps=16'],
 'plain':['-O2','-falign-jumps=1'],
 'j1-l4':['-O2','-frename-registers','-falign-jumps=1','-falign-labels=4'],
 'j4-l1':['-O2','-frename-registers','-falign-jumps=4','-falign-labels=1'],
}
T=Path('/tmp/vuc-binutils215/value-plus-goto-natural-order-oct15e');T.mkdir(parents=True,exist_ok=True)
raw,elf=m.decomp.load();sec=next(s for s in elf['sections'] if s['name']=='.text');addr=int(cfg['address'],0);ref=raw[sec['offset']+addr-sec['address']:sec['offset']+addr-sec['address']+cfg['size']]
results=[];sizes=collections.Counter();best=None
for (name,pattern),(fn,opts) in itertools.product(forms.items(),flags.items()):
 label=('$END' in init)
 code=init.replace('$BODY',pattern.replace('$VEC',vector).replace('$INT',frags['int']).replace('$SCALAR',frags['scalar']))
 code=code.replace('$END','done:\n    return result;' if 'goto' in name or name=='switch-break-to-return' or name=='switch-literal-ordered' or name=='switch-literal-swap' else 'return result;')
 # If a case contains goto done but our template has it, it still has a scope-valid named label.
 candidate=base[:start]+code+base[end:]
 tag=name+'-'+fn
 cpp=T/(tag+'.cpp');obj=T/(tag+'.o');cpp.write_text(candidate)
 p=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if p.returncode:results.append(dict(tag=tag,error=p.stderr[-145:]));continue
 try:b,r=m.prepare_named_function(obj.read_bytes(),cfg['function'],cfg['relocations'],addr)
 except ValueError as e:results.append(dict(tag=tag,error=str(e)));continue
 score=sum(x==y for x,y in zip(ref,b));prefix=next((i for i,(x,y) in enumerate(zip(ref,b)) if x!=y),min(len(ref),len(b)))
 row=dict(tag=tag,length=len(b),match=score,prefix=prefix,relocations=r)
 results.append(row);sizes[len(b)]+=1
 if len(b)==cfg['size'] and r==1 and (best is None or (score,prefix)>(best['match'],best['prefix'])):
  best=row;(T/'best.cpp').write_text(candidate);(T/'best.flags').write_text(' '.join(opts)+'\n');(T/'best.o').write_bytes(obj.read_bytes())
  if score==cfg['size']:
   print('EXACT',json.dumps(row),flush=True)
   (T/'results.json').write_text(json.dumps(results,indent=2)+'\n')
   raise SystemExit(0)
(T/'results.json').write_text(json.dumps(results,indent=2)+'\n')
print(json.dumps(dict(attempted=len(results),sizes=dict(sizes),best=best,top=sorted((r for r in results if 'match' in r),key=lambda x:(x['match'],x['prefix']),reverse=True)[:13]),indent=2))

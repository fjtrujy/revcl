#!/usr/bin/env python3
"""VUC Value::operator&& genuine C++03 goto/return topology controls.
The selected source is original-length 325 bytes / 192 original offsets.
The successful IsAccumulatorable negative-guard case taught that explicit
source fallthrough to the original branch target can change GCC i386 CFG.
Test that *source-level* technique here, with the original float comparisons.
No asm, handpadding, hard registers, synthetic function targets.
"""
from pathlib import Path
import itertools,importlib.util,json,subprocess,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
conf=json.loads((R/'vuc/config/value_logical_and_partial_oct14_probe.json').read_text())
base=(R/conf['source']).read_text()
s=base.index('Value Value::operator&&(const Value &other) const {')
e=base.index('}',s)
# closing function is last in candidate; use rfind
e=base.rfind('}')+1
original_body=base[s:e]
vector='result.bits=!((result.values[1] == 0.0f) || (result.values[0] == 0.0f) || (result.values[2] == 0.0f) || (result.values[3] == 0.0f) || (temporary.values[0] == 0.0f) || (temporary.values[1] == 0.0f) || (temporary.values[2] == 0.0f) || (temporary.values[3] == 0.0f));'
integer='result.bits=(result.bits && temporary.bits);'
scalar='result.bits=(result.values[0] != 0.0f && temporary.values[0] != 0.0f);'
source_header='''Value Value::operator&&(const Value &other) const {
 Value result;
 Value temporary;
 result.type=2;
 $GATE
 $DISPATCH
done:
 return result;
}
'''
patterns={
 'original':'''switch(result.type){
case 1:case 2:$INT break;
case 3:$SCALAR break;
case 4:$VEC break;
}''',
 'goto-case-tail':'''switch(result.type){
case 1:case 2:$INT goto done;
case 3:$SCALAR goto done;
case 4:$VEC goto done;
}''',
 'goto-scalar':'''switch(result.type){
case 1:case 2:$INT goto done;
case 3:goto scalar;
case 4:$VEC goto done;
}
goto done;
scalar:$SCALAR goto done;''',
 'goto-vector':'''switch(result.type){
case 1:case 2:$INT goto done;
case 3:$SCALAR goto done;
case 4:goto vector;
}
goto done;
vector:$VEC goto done;''',
 'goto-both':'''switch(result.type){
case 1:case 2:goto integer;
case 3:goto scalar;
case 4:goto vector;
}
goto done;
integer:$INT goto done;
scalar:$SCALAR goto done;
vector:$VEC goto done;''',
 'goto-reversed':'''switch(result.type){
case 4:goto vector;
case 3:goto scalar;
case 1:case 2:goto integer;
}
goto done;
vector:$VEC goto done;
scalar:$SCALAR goto done;
integer:$INT goto done;''',
 'if-type':'''if(result.type==1 || result.type==2){$INT goto done;}
if(result.type==3){$SCALAR goto done;}
if(result.type==4){$VEC goto done;}''',
 'if-vectorfirst':'''if(result.type==4){$VEC goto done;}
if(result.type==3){$SCALAR goto done;}
if(result.type==1 || result.type==2){$INT goto done;}''',
 'if-scalarfirst':'''if(result.type==3){$SCALAR goto done;}
if(result.type==4){$VEC goto done;}
if(result.type==1 || result.type==2){$INT goto done;}''',
 'negative-vector':'''if(result.type!=4)goto notvector;
$VEC goto done;
notvector:
if(result.type==3){$SCALAR goto done;}
if(result.type==1 || result.type==2){$INT goto done;}''',
 'negative-scalar':'''if(result.type!=3)goto notscalar;
$SCALAR goto done;
notscalar:
if(result.type==4){$VEC goto done;}
if(result.type==1 || result.type==2){$INT goto done;}''',
 'negative-integer':'''if(result.type!=1 && result.type!=2)goto notinteger;
$INT goto done;
notinteger:
if(result.type==3){$SCALAR goto done;}
if(result.type==4){$VEC goto done;}''',
 'switch-jumpout':'''switch(result.type){
case 1:case 2:$INT break;
case 3:$SCALAR break;
case 4:$VEC break;
}
goto done;''',
}
gates={
 'outer-if':'if(TypeConvert(*this,other,result,temporary)) { $DISPATCH }',
 'early-negative':'if(!TypeConvert(*this,other,result,temporary))goto done; $DISPATCH',
 'outer-positive':'if(TypeConvert(*this,other,result,temporary))goto work;goto done;work:$DISPATCH',
}
flags={
 'saved':['-O2','-frename-registers','-ffast-math','-falign-jumps=16','-falign-labels=1','-falign-loops=1'],
 'j1':['-O2','-frename-registers','-ffast-math','-falign-jumps=1','-falign-labels=1','-falign-loops=1'],
 'j4':['-O2','-frename-registers','-ffast-math','-falign-jumps=4','-falign-labels=1','-falign-loops=1'],
 'j8':['-O2','-frename-registers','-ffast-math','-falign-jumps=8','-falign-labels=1','-falign-loops=1'],
 'j16-nocross':['-O2','-frename-registers','-ffast-math','-falign-jumps=16','-falign-labels=1','-falign-loops=1','-fno-crossjumping'],
 'j16-noreorder':['-O2','-frename-registers','-ffast-math','-falign-jumps=16','-falign-labels=1','-falign-loops=1','-fno-reorder-blocks'],
}
T=Path('/tmp/vuc-binutils215/value-and-goto-negative-topology-oct15e');T.mkdir(parents=True,exist_ok=True)
raw,elf=m.decomp.load();sec=next(x for x in elf['sections'] if x['name']=='.text');addr=int(conf['address'],0);ref=raw[sec['offset']+addr-sec['address']:sec['offset']+addr-sec['address']+325]
rows=[];sizes=collections.Counter();best=None
for (dn,dispatch),(gn,gate),(fn,opts) in itertools.product(patterns.items(),gates.items(),flags.items()):
 tag=dn+'-'+gn+'-'+fn
 sub=dispatch.replace('$INT',integer).replace('$SCALAR',scalar).replace('$VEC',vector)
 source=source_header.replace('$GATE',gate.replace('$DISPATCH',sub)).replace('$DISPATCH','')
 candidate=base[:s]+source
 cpp=T/(tag+'.cpp');obj=T/(tag+'.o');cpp.write_text(candidate)
 p=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if p.returncode:rows.append(dict(tag=tag,error=p.stderr[-165:]));continue
 try:b,r=m.prepare_named_function(obj.read_bytes(),conf['function'],conf['relocations'],addr)
 except ValueError as e:rows.append(dict(tag=tag,error=str(e)[:175]));continue
 score=sum(x==y for x,y in zip(ref,b));prefix=next((i for i,(x,y) in enumerate(zip(ref,b)) if x!=y),min(len(ref),len(b)))
 row=dict(tag=tag,length=len(b),match=score,prefix=prefix,relocations=r)
 rows.append(row);sizes[len(b)]+=1
 if len(b)==325 and r==1 and (best is None or (score,prefix)>(best['match'],best['prefix'])):
  best=row;(T/'best.cpp').write_text(candidate);(T/'best.flags').write_text(' '.join(opts)+'\n');(T/'best.o').write_bytes(obj.read_bytes())
  if score>192:print('IMPROVE',json.dumps(row),flush=True)
  if score==325:
   print('EXACT',json.dumps(row),flush=True);break
(T/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
print(json.dumps(dict(attempted=len(rows),sizes=dict(sizes),best=best,top=sorted((r for r in rows if 'match' in r),key=lambda r:(r['match'],r['prefix']),reverse=True)[:15]),indent=2))

#!/usr/bin/env python3
"""Reconstruct previously unresolved VUC Value::operator&& and operator||.

C++03 source is derived from existing accepted Value relational operators
and the original ELF's int/float4 logical comparison paths. Authenticated
TypeConvert relocation, original-sized code and original instruction bytes
are prerequisites for selecting any partial candidate.
"""
from pathlib import Path
import itertools,importlib.util,subprocess,json,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('matcher',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
raw,elf=m.decomp.load();sec=next(s for s in elf['sections'] if s['name']=='.text')
targets={
 'and':dict(symbol='_ZNK5ValueaaERKS_',address=0x0806f340,size=325),
 'or':dict(symbol='_ZNK5ValueooERKS_',address=0x0806f490,size=309),
}
relocations={'_ZN5Value11TypeConvertERKS_S1_RS_S2_':'0x0806f7d0'}
header='''// Authentic Value ABI derived from the independently exact relational unit.
struct Value {
 int type; unsigned a,b; unsigned char flag0,flag1,flag2,flag3;
 union { int bits; float values[4]; };
 Value() : type(0),a(0),b(0),flag0(0),flag1(0),flag2(0),flag3(0) {}
 static bool TypeConvert(const Value&,const Value&,Value&,Value&);
 Value operator&&(const Value&) const;
 Value operator||(const Value&) const;
};
'''
intpred={
 'truth':'(result.bits {op} temporary.bits)',
 'neq':'((result.bits!=0) {op} (temporary.bits!=0))',
 'bool':'(bool(result.bits) {op} bool(temporary.bits))',
}
flt={
 'neq':'(result.values[{i}] != 0.0f {op} temporary.values[{i}] != 0.0f)',
 'bool':'(bool(result.values[{i}]) {op} bool(temporary.values[{i}]))',
 'neg-eq':'(!(result.values[{i}] == 0.0f) {op} !(temporary.values[{i}] == 0.0f))',
}
vcond={
 'chained':' {op} '.join('({p'+str(i)+'})' for i in range(4)),
 'short':'({p0}) {op} ({p1}) {op} ({p2}) {op} ({p3})',
}
cases={'fall':'case 1: case 2:','separate':'case 1: case 2:'}
flags={
 'ffast':['-O2','-frename-registers','-ffast-math'],
 'ffast-j1':['-O2','-frename-registers','-ffast-math','-falign-jumps=1'],
 'ffast-j4':['-O2','-frename-registers','-ffast-math','-falign-jumps=4'],
 'no-ffast':['-O2','-frename-registers'],
 'plain':['-O2','-ffast-math'],
}
T=Path('/tmp/vuc-binutils215/value-logical-reconstruct-next-oct14');T.mkdir(parents=True,exist_ok=True)
scores={k:[] for k in targets};counts={k:collections.Counter() for k in targets};best={k:None for k in targets}
for logical,(intk,integer),(floatk,floatcond),mode,flag in itertools.product(
 (('and','&&'),('or','||')),intpred.items(),flt.items(),vcond,flags):
 k,op=logical
 name=f'{k}-{intk}-{floatk}-{mode}-{flag}'
 predicates={f'p{i}':floatcond.format(i=i,op=op) for i in range(4)}
 expression=vcond[mode].format(op=op,**predicates)
 source=header+f'''
Value Value::operator{op}(const Value &other) const {{
 Value result;
 Value temporary;
 result.type=2;
 if (TypeConvert(*this,other,result,temporary)) {{
  switch (result.type) {{
   case 1: case 2: result.bits={intpred[intk].format(op=op)};break;
   case 3: result.bits={floatcond.format(i=0,op=op)};break;
   case 4: result.bits={expression};break;
  }}
 }}
 return result;
}}
'''
 cpp=T/(name+'.cpp');obj=T/(name+'.o');cpp.write_text(source)
 cc=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*flags[flag],'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if cc.returncode:
  scores[k].append(dict(name=name,error=cc.stderr[-140:]));continue
 cfg=targets[k]
 try:code,rel=m.prepare_named_function(obj.read_bytes(),cfg['symbol'],relocations,cfg['address'])
 except ValueError as e:
  scores[k].append(dict(name=name,error=str(e)));continue
 ref=raw[sec['offset']+cfg['address']-sec['address']:sec['offset']+cfg['address']-sec['address']+cfg['size']]
 count=sum(a==b for a,b in zip(code,ref));prefix=next((i for i,(a,b) in enumerate(zip(code,ref)) if a!=b),min(len(code),len(ref)))
 row=dict(name=name,length=len(code),relocations=rel,overlap=count,prefix=prefix)
 scores[k].append(row);counts[k][len(code)]+=1
 if len(code)==cfg['size'] and rel==1 and (best[k] is None or (count,prefix)>(best[k]['overlap'],best[k]['prefix'])):
  best[k]=row
  (T/f'best-{k}.cpp').write_text(source);(T/f'best-{k}.o').write_bytes(obj.read_bytes())
(T/'results.json').write_text(json.dumps(scores,indent=2)+'\n')
for k in targets:
 print(json.dumps(dict(target=k,attempted=len(scores[k]),sizes=dict(counts[k]),best_original_length=best[k],top=sorted((x for x in scores[k] if 'overlap' in x),key=lambda x:(x['overlap'],x['prefix']),reverse=True)[:5]),indent=2),flush=True)

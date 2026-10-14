#!/usr/bin/env python3
"""VUC original Value+ 219-byte selected lane-order operand ownership.

After real lane order 3,1,2,0 improves 172->203 original-offset bytes,
test independent scalar and integer operand spelling, and each lane's
source-level commutative float addition, without ASM or synthetic padding.
"""
from pathlib import Path
import importlib.util,itertools,subprocess,json,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location("m",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=json.loads((R/"vuc/config/value_addition_partial_oct14_probe.json").read_text())
raw,elf=m.decomp.load();s=next(x for x in elf["sections"] if x["name"]==".text");addr=int(cfg["address"],0)
ref=raw[s["offset"]+addr-s["address"]:s["offset"]+addr-s["address"]+219]
src=(R/"vuc/src/ValueArithmeticUnit.cpp").read_text()
first=src.index("Value Value::operator+(const Value &other) const");last=src.index("Value Value::operator-(",first)
op=src[first:last]
scalarold="case 3: result.values[0] += temporary.values[0]; break;"
intold="case 1: result.bits += temporary.bits; break;"
vecold="""case 4:
            result.values[0] += temporary.values[0];
            result.values[1] += temporary.values[1];
            result.values[2] += temporary.values[2];
            result.values[3] += temporary.values[3];
            break;"""
ints={
 "normal":intold,
 "reverse":"case 1: result.bits = temporary.bits + result.bits; break;",
 "local":"case 1: { unsigned rhs=temporary.bits; result.bits += rhs; break; }",
}
scalars={
 "normal":scalarold,
 "assign":"case 3: result.values[0] = result.values[0] + temporary.values[0]; break;",
 "reverse":"case 3: result.values[0] = temporary.values[0] + result.values[0]; break;",
 "local":"case 3: { float rhs=temporary.values[0]; result.values[0]+=rhs; break; }",
}
orders=((3,1,2,0),(1,3,2,0),(3,2,1,0))
flags={
 'j1':['-O2','-frename-registers','-falign-jumps=1'],
 'j2':['-O2','-frename-registers','-falign-jumps=2'],
}
T=Path("/tmp/vuc-binutils215/value-plus-203-float-scalar-controls-oct15");T.mkdir(parents=True,exist_ok=True)
rows=[];sizes=collections.Counter();best=None
for order,(ik,intform),(sk,scalarform),mask,(fk,opts) in itertools.product(orders,ints.items(),scalars.items(),range(16),flags.items()):
 tag=''.join(map(str,order))+f'-{ik}-{sk}-m{mask}-{fk}'
 terms=[]
 for i in order:
  if mask&(1<<i):line=f"            result.values[{i}] = temporary.values[{i}] + result.values[{i}];"
  else:line=f"            result.values[{i}] += temporary.values[{i}];"
  terms.append(line)
 repl="case 4:\n"+"\n".join(terms)+"\n            break;"
 code=src.replace(op,op.replace(intold,intform).replace(scalarold,scalarform).replace(vecold,repl))
 cpp=T/(tag+'.cpp');obj=T/(tag+'.o');cpp.write_text(code)
 p=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if p.returncode:rows.append(dict(tag=tag,error=p.stderr[-110:]));continue
 try:b,r=m.prepare_named_function(obj.read_bytes(),cfg["function"],cfg["relocations"],addr)
 except ValueError as e:rows.append(dict(tag=tag,error=str(e)));continue
 score=sum(a==x for a,x in zip(ref,b))
 prefix=next((i for i,(a,x) in enumerate(zip(ref,b)) if a!=x),min(len(ref),len(b)))
 row=dict(tag=tag,length=len(b),relocations=r,match=score,prefix=prefix)
 rows.append(row);sizes[len(b)]+=1
 if len(b)==219 and r==1 and (best is None or (score,prefix)>(best["match"],best["prefix"])):
  best=row;(T/'best.cpp').write_text(code);(T/'best.flags').write_text(' '.join(opts)+'\n');(T/'best.o').write_bytes(obj.read_bytes())
(T/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
print(json.dumps(dict(attempted=len(rows),sizes=dict(sizes),best=best,top=sorted((x for x in rows if "match" in x),key=lambda r:(r["match"],r["prefix"]),reverse=True)[:14]),indent=2))

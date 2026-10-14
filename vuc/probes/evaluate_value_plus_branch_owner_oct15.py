#!/usr/bin/env python3
"""Original Value addition branch placement and GCC 3.3 source switch evidence.

The original 219-byte function has vector block at +0xa2 but scalar case at
+0xd0, unlike uniform -falign-jumps padding. Recover genuine source branch
likelihood and real switch case ownership without fabricated padding/asm.
"""
from pathlib import Path
import importlib.util,itertools,json,subprocess,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location("m",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=json.loads((R/"vuc/config/value_addition_partial_oct14_probe.json").read_text())
raw,elf=m.decomp.load();s=next(x for x in elf["sections"] if x["name"]==".text")
addr=int(cfg["address"],0);ref=raw[s["offset"]+addr-s["address"]:s["offset"]+addr-s["address"]+219]
body=(R/"vuc/src/ValueArithmeticUnit.cpp").read_text()
start=body.index("Value Value::operator+(const Value &other) const")
end=body.index("Value Value::operator-(",start)
prefix=body[:start];suffix=body[end:]
block=body[start:end]
startswitch=block.index("        switch (result.type) {")
endswitch=block.index("        }\n    }\n    return result;",startswitch)+len("        }")
caseparts={
 'int':'case 1: result.bits += temporary.bits; break;',
 'scalar':'case 3: result.values[0] += temporary.values[0]; break;',
 'vector':'case 4:\n            result.values[0] += temporary.values[0];\n            result.values[1] += temporary.values[1];\n            result.values[2] += temporary.values[2];\n            result.values[3] += temporary.values[3];\n            break;',
}
predicates={
 'natural':"result.type",
 'expect1':"__builtin_expect(result.type,1)",
 'expect3':"__builtin_expect(result.type,3)",
 'expect4':"__builtin_expect(result.type,4)",
 'expect0':"__builtin_expect(result.type,0)",
}
modes=('switch','independent-if','ifelse')
flags={
 'j4':['-O2','-frename-registers','-falign-jumps=4'],
 'j4l1':['-O2','-frename-registers','-falign-jumps=4','-falign-labels=1'],
 'j1':['-O2','-frename-registers','-falign-jumps=1'],
 'j8':['-O2','-frename-registers','-falign-jumps=8'],
}
T=Path("/tmp/vuc-binutils215/value-plus-control-recipe-next-oct15");T.mkdir(parents=True,exist_ok=True)
rank=[];lengths=collections.Counter();best=None
for order,mode,(name,expr),(fl,opts) in itertools.product(itertools.permutations(caseparts),modes,predicates.items(),flags.items()):
 key="-".join(order)+f"-{mode}-{name}-{fl}"
 if mode=='switch':region="        switch ("+expr+") {\n"+"\n".join("        "+caseparts[k] for k in order)+"\n        }"
 else:
  conditions={'int':'('+expr+' == 1)','scalar':'('+expr+' == 3)','vector':'('+expr+' == 4)'}
  expressions={'int':'result.bits += temporary.bits;', 'scalar':'result.values[0] += temporary.values[0];','vector':' '.join(f'result.values[{i}] += temporary.values[{i}];' for i in range(4))}
  joiner="\n        else " if mode=='ifelse' else "\n        "
  region="        "+joiner.join("if "+conditions[k]+" { "+expressions[k]+" }" for k in order)
 cpp_text=prefix+block[:startswitch]+region+block[endswitch:]+suffix
 cpp=T/(key+".cpp");obj=T/(key+".o");cpp.write_text(cpp_text)
 cc=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*opts,"-c",str(cpp),"-o",str(obj)],capture_output=True,text=True)
 if cc.returncode:rank.append(dict(key=key,error=cc.stderr[-170:]));continue
 try:code,rel=m.prepare_named_function(obj.read_bytes(),cfg["function"],cfg["relocations"],addr)
 except ValueError as e:rank.append(dict(key=key,error=str(e)));continue
 score=sum(a==b for a,b in zip(ref,code))
 exact_prefix=next((i for i,(a,b) in enumerate(zip(ref,code)) if a!=b),min(len(ref),len(code)))
 r=dict(key=key,length=len(code),relocations=rel,match=score,prefix=exact_prefix)
 rank.append(r);lengths[len(code)]+=1
 if len(code)==219 and rel==1 and (best is None or (score,exact_prefix)>(best["match"],best["prefix"])):
  best=r;(T/"best.cpp").write_text(cpp_text);(T/"best.flags").write_text(" ".join(opts)+"\n");(T/"best.o").write_bytes(obj.read_bytes())
(T/"results.json").write_text(json.dumps(rank,indent=2)+"\n")
print(json.dumps(dict(attempted=len(rank),sizes=dict(lengths),best=best,top=sorted((r for r in rank if "match" in r),key=lambda r:(r["match"],r["prefix"]),reverse=True)[:12]),indent=2))

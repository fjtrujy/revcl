#!/usr/bin/env python3
"""Use normal C++03 Type copy semantics to investigate Check(ASTFunCall*) tail."""
from pathlib import Path
import importlib.util,subprocess
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215")
sp=importlib.util.spec_from_file_location("m",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
base=(R/"vuc/probes/check_ast_fun_call.cpp").read_text()
old="    Type(const Type &other) : a(other.a), b(other.b), c(other.c), d(other.d) {}"
assert old in base
cases={
 "implicit":"",
 "body-forward":"    Type(const Type &other) { a=other.a; b=other.b; c=other.c; d=other.d; }",
 "body-backward":"    Type(const Type &other) { d=other.d; c=other.c; b=other.b; a=other.a; }",
 "body-pairs":"    Type(const Type &other) { a=other.a; b=other.b; c=other.c; d=other.d; }",
 "ctor-default":"    Type(const Type &other) : a(other.a), b(other.b), c(other.c), d(other.d) {}",
 "only-a":"    Type(const Type &other) : a(other.a), b(other.b), c(other.c), d(other.d) {}",
 "memcpy":"    Type(const Type &other) { __builtin_memcpy(this, &other, sizeof(Type)); }",
 "memcpy-16":"    Type(const Type &other) { __builtin_memcpy(this, &other, 16); }",
 "pointer":"    Type(const Type &other) { const unsigned *in=(const unsigned*)&other; unsigned *out=(unsigned*)this; for(int i=0;i<4;i++) out[i]=in[i]; }",
}
flags={
"baseline":["-O2","-frename-registers"],
"no-sched":["-O2","-frename-registers","-fno-schedule-insns2"],
"no-gcse":["-O2","-frename-registers","-fno-gcse"],
"no-strict":["-O2","-frename-registers","-fno-strict-aliasing"],
"plain":["-O2"],
}
results=[]
for name,ctor in cases.items():
 p=T/("checkfun-copy-"+name+".cpp")
 p.write_text(base.replace(old,ctor).replace('../src/',''))
 for option,opts in flags.items():
  obj=T/("checkfun-copy-"+name+"-"+option+".o")
  cp=subprocess.run([str(T/"g++-jan25-as215"),*opts,"-I",str(R/'vuc/src'),"-c",str(p),"-o",str(obj)],capture_output=True,text=True)
  if cp.returncode:results.append((0,name,option,cp.stderr[-85:]));continue
  try:
   item=m.run(obj,R/"vuc/config/check_ast_fun_call_probe.json")
   results.append((item["exact_bytes"],name,option,str(item["length"])))
  except Exception as e:results.append((0,name,option,str(e)[:90]))
  if results[-1][0]==118:
   print("EXACT",name,option,flush=True)
   break
results.sort(reverse=True)
for line in results[:25]:print(*line)

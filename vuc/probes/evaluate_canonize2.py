#!/usr/bin/env python3
"""Source-form selection for VUC module reduction without register forcing."""
import importlib.util,subprocess
from pathlib import Path
R=Path(__file__).resolve().parents[2];T=Path("/tmp/vuc-binutils215")
sp=importlib.util.spec_from_file_location("m",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
base=(R/"vuc/probes/canonize2_reduce.cpp").read_text()
assert '    if (module->flag) return;' in base
forms={
"direct":"    if (module->flag) return;",
"not-zero":"    if (module->flag != 0) return;",
"zero-reverse":"    if (0 != module->flag) return;",
"bool-conversion":"    if ((bool)module->flag) return;",
"local-int":"    int const flag=module->flag;\n    if(flag) return;",
"local-unsigned":"    unsigned const flag=module->flag;\n    if(flag) return;",
"inline-method":"    if (module->IsComplete()) return;",
"inline-function":"    if (module->is_complete()) return;",
"volatile-method":"    if (module->IsCompleteVolatile()) return;",
"switch":"    switch(module->flag) { case 0: break; default: return; }",
"plus-zero":"    if(module->flag+0) return;",
"return-expr":"    if(!!module->flag) return;",
"compare-one":"    if(module->flag > 0) return;",
}
header_methods='''        bool IsComplete() const { return flag; }
        int is_complete() const { return flag; }
        int IsCompleteVolatile() const { volatile int const *p=&flag; return *p; }
'''
r=[]
for label,body in forms.items():
 text=base.replace('    if (module->flag) return;',body)
 if label in ("inline-method","inline-function","volatile-method"):
  text=text.replace('        int flag;\n','        int flag;\n'+header_methods)
 p=T/("canonical-"+label+".cpp");p.write_text(text)
 for option,opts in [('renj1',['-O2','-frename-registers','-falign-jumps=1']),('plainj1',['-O2','-falign-jumps=1'])]:
  obj=T/("canonical-"+label+"-"+option+".o")
  cp=subprocess.run([str(T/"g++-jan25-as215"),*opts,'-I',str(R/'vuc/src'),'-c',str(p),'-o',str(obj)],capture_output=True,text=True)
  if cp.returncode:r.append((0,label,option,cp.stderr[-70:]));continue
  try:
   s=m.run(obj,R/'vuc/config/canonize2_reduce_probe.json')
   r.append((s['exact_bytes'],label,option,str(s['length'])))
  except Exception as e:r.append((0,label,option,str(e)[:75]))
r.sort(reverse=True)
for a in r:print(*a)

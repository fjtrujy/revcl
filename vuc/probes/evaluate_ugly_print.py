#!/usr/bin/env python3
"""Recover original Printer_UglyPrint register selection without handwritten asm."""
from pathlib import Path
import importlib.util, subprocess
ROOT=Path(__file__).resolve().parents[2];TMP=Path("/tmp/vuc-binutils215")
sp=importlib.util.spec_from_file_location("probe",ROOT/"vuc/scripts/match_probe.py")
mod=importlib.util.module_from_spec(sp);sp.loader.exec_module(mod)
raw=(ROOT/"vuc/probes/ugly_print.cpp").read_text()
needle='    Print(module->declarations_, output, "\\n");'
assert needle in raw
shapes={
"direct":needle,
"temp-sep":'    const char *sep = "\\n";\n    Print(module->declarations_, output, sep);',
"temp-refs":'    ASTNodeList<ASTDecl *> &list = module->declarations_;\n    Print(list, output, "\\n");',
"temp-both":'    ASTNodeList<ASTDecl *> &list = module->declarations_;\n    const char *sep = "\\n";\n    Print(list, output, sep);',
"return-print":'    return Print(module->declarations_, output, "\\n");',
"cast":'    Print(module->declarations_, output, (const char *)"\\n");',
"ptr-address":'    ASTNodeList<ASTDecl *> *list = &module->declarations_;\n    Print(*list, output, "\\n");',
"comma":'    Print(module->declarations_, output, (0, "\\n"));',
"by-ptr":'    const char *sep = "\\n";\n    ASTNodeList<ASTDecl *> *list=&module->declarations_;\n    Print(*list,output,sep);'
}
flags={
"ren":["-O2","-frename-registers"],
"ren-j1":["-O2","-frename-registers","-falign-jumps=1"],
"ren-nortti":["-O2","-frename-registers","-fno-rtti"],
"ren-noexceptions":["-O2","-frename-registers","-fno-exceptions"],
"ren-noinline":["-O2","-frename-registers","-fno-inline"],
"ren-fno-cse":["-O2","-frename-registers","-fno-cse-follow-jumps"],
"ren-gnu89":["-O2","-frename-registers","-fno-strict-aliasing"],
"ren-march-i686":["-O2","-frename-registers","-march=i686"],
"ren-noregmove":["-O2","-frename-registers","-fno-regmove"],
}
out=[]
for name,shape in shapes.items():
 src=TMP/("ugly-shape-"+name+".cpp");src.write_text(raw.replace(needle,shape))
 for fkey,opts in flags.items():
  obj=TMP/("ugly-shape-"+name+"-"+fkey+".o")
  cp=subprocess.run([str(TMP/"g++-jan25-as215"),*opts,"-c",str(src),"-o",str(obj)],capture_output=True,text=True)
  if cp.returncode:
   out.append((0,name,fkey,cp.stderr[-80:]));continue
  try:
   result=mod.run(obj,ROOT/"vuc/config/ugly_print_probe.json")
   out.append((result["exact_bytes"],name,fkey,str(result["length"])))
  except Exception as e:out.append((0,name,fkey,str(e)[:60]))
out.sort(reverse=True)
for x in out[:25]:print(*x)

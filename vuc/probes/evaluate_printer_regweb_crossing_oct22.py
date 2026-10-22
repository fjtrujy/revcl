#!/usr/bin/env python3
"""Cross-region genuine callee/arguments value-lifetime controls for VUC Print(ASTFunCall*).

Only inspect accepted original-size outputs with all relocations and source literals
authenticated by match_probe. Save complete generated original-era C++ and scores
under /tmp; never rewrite the accepted Printer.cpp source.
"""
from pathlib import Path
import hashlib, importlib.util, itertools, json, subprocess
ROOT = Path(__file__).resolve().parents[1]
OWNER = ROOT / "src/Printer.cpp"
TARGET = ROOT / "config/print_ast_fun_context_oct14_probe.json"
WORK = Path("/tmp/vuc-printer-arg-web-20261022")
WORK.mkdir(parents=True,exist_ok=True)
base = OWNER.read_text().replace('#include "../src/ASTNodeList.h"',
                                 '#include "' + str(ROOT / "src/ASTNodeList.h") + '"')
old = '''    Print(call->callee, output);
    output << "(";
    Print(call->arguments, output, ", ");
    output << ")";
'''
assert base.count(old)==1
sp=importlib.util.spec_from_file_location("vuc_match_probe_oct22",ROOT/"scripts/match_probe.py")
match=importlib.util.module_from_spec(sp);sp.loader.exec_module(match)
callee_forms={
  "direct": ("", "call->callee"),
  "value": ("ASTLValExpr *target = call->callee;", "target"),
  "const-value": ("ASTLValExpr *const target = call->callee;", "target"),
  "slot": ("ASTLValExpr **target = &call->callee;", "*target"),
  "reference": ("ASTLValExpr *&target = call->callee;", "target"),
  "base-ref": ("ASTFunCall &c = *call;", "c.callee"),
  "base-ptr": ("ASTFunCall *c = call;", "c->callee"),
  "field-offset": ("char *storage = reinterpret_cast<char *>(call);", "*reinterpret_cast<ASTLValExpr **>(storage+8)"),
}
argument_forms={
  "direct": ("", "call->arguments"),
  "local-ref": ("ASTNodeList<ASTExpr *> &items = call->arguments;", "items"),
  "local-ptr": ("ASTNodeList<ASTExpr *> *items = &call->arguments;", "*items"),
  "arg-c": ("ASTFunCall &argsCall=*call;", "argsCall.arguments"),
}
positions=("pre","post")
results=[]
for (ck,(cdef,cval)),(ak,(adef,aval)),pos in itertools.product(callee_forms.items(),argument_forms.items(),positions):
  # The arguments member is a real value used by the later Print call.
  # Move its address across the first callee and ostream insertion calls.
  defs="    " + ("\n    ".join(x for x in (adef,cdef) if x)) if pos=="pre" else "    "+cdef
  if pos=="post": first=defs + "\n    Print("+cval+", output);\n    output << \"(\";\n    "+adef
  else: first=defs + "\n    Print("+cval+", output);\n    output << \"(\";"
  replacement=first+"\n    Print("+aval+', output, ", ");\n    output << ")";\n'
  if not defs.strip():replacement=replacement.lstrip("\n")
  tag=f"{ck}__{ak}__{pos}"
  source=base.replace(old,replacement)
  path=WORK/(tag+".cpp"); path.write_text(source)
  record={"case":tag,"source_sha256":hashlib.sha256(source.encode()).hexdigest()}
  for snapshot in ("jan25","dec06"):
    obj=WORK/(tag+"-"+snapshot+".o")
    cmd=["/tmp/vuc-binutils215/g++-"+snapshot+"-as215","-O2","-c",str(path),"-o",str(obj)]
    compiled=subprocess.run(cmd,capture_output=True,text=True)
    if compiled.returncode: record[snapshot]={"error":compiled.stderr[-400:]};continue
    try: record[snapshot]=match.run(obj,TARGET)
    except Exception as exc:record[snapshot]={"error":str(exc)}
    if record[snapshot].get("exact"): print("EXACT",tag,snapshot,flush=True)
  results.append(record)
  if len(results)%16==0:print("completed",len(results),flush=True)
(WORK/"results.json").write_text(json.dumps(results,indent=2)+"\n")
by={}
for snapshot in ("jan25","dec06"):
  valid=[r for r in results if "exact_bytes" in r.get(snapshot,{}) and r[snapshot].get("length")==98]
  by[snapshot]={"compiled":sum("error" not in r.get(snapshot,{}) for r in results),
                "original_length":len(valid),
                "best":max([r[snapshot]["exact_bytes"] for r in valid],default=0),
                "best_cases":[r["case"] for r in valid if r[snapshot]["exact_bytes"] == max(v[snapshot]["exact_bytes"] for v in valid)][:10],
                "exact":sum(r[snapshot]["exact"] for r in valid),
                "offset_sets":sorted({str(r[snapshot]["different_offsets"]) for r in valid})}
print(json.dumps(by,indent=2),flush=True)

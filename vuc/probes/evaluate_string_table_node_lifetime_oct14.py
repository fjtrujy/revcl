#!/usr/bin/env python3
"""Test genuine pointer/reference lifetime around VUC string intern node new."""
from pathlib import Path
import subprocess,importlib.util
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/string-node-lifetime-oct14");T.mkdir(exist_ok=True,parents=True)
sp=importlib.util.spec_from_file_location("match",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
original=(R/"vuc/probes/string_table_add_source_oct14.cpp").read_text()
line="    Table::Node *node = new Table::Node(data->text, data);"
cases={
"baseline":line,
"const-ref":"    const char *const &name = data->text;\n    Table::Node *node = new Table::Node(name, data);",
"ref":"    const char *&name = data->text;\n    Table::Node *node = new Table::Node(name, data);",
"pointer":"    const char **name = &data->text;\n    Table::Node *node = new Table::Node(*name, data);",
"explicit-address":"    const char *const *name = &data->text;\n    Table::Node *node = new Table::Node(*name, data);",
"key-value-local":"    const char *name = data->text;\n    Table::Node *node = new Table::Node(name, data);",
"local-int":"    unsigned h = data->hash;\n    Table::Node *node = new Table::Node(data->text, data);\n    (void)h;",
"data-ref":"    MLGameStringData *const &current = data;\n    Table::Node *node = new Table::Node(current->text, current);",
"local-data":"    MLGameStringData *const current = data;\n    Table::Node *node = new Table::Node(current->text, current);",
"parenthesized":"    Table::Node *node = new Table::Node((data->text), (data));",
}
flags={
"r":["-O2","-frename-registers"],
"rj1":["-O2","-frename-registers","-falign-jumps=1"],
"rj2":["-O2","-frename-registers","-falign-jumps=2"],
"rj4":["-O2","-frename-registers","-falign-jumps=4"],
"rj8":["-O2","-frename-registers","-falign-jumps=8"],
"nr":["-O2"],
}
results=[]
for name,body in cases.items():
 path=T/(name+".cpp");path.write_text(original.replace(line,body))
 for flag,options in flags.items():
  obj=T/(name+"-"+flag+".o")
  c=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*options,"-c",str(path),"-o",str(obj)],capture_output=True,text=True)
  if c.returncode:print("ERROR",name,flag,c.stderr[-90:],flush=True);continue
  sym=next((x for x in subprocess.check_output(["nm","-S",str(obj)],text=True).splitlines() if x.endswith("_ZN17MLGameStringTable3AddEPKcb")),"")
  size=int(sym.split()[1],16) if sym else -1
  if size!=263:results.append((0,size,name,flag));continue
  try:
   s=m.run(obj,R/"vuc/config/string_table_add_oct14_probe.json")
   results.append((s["exact_bytes"],size,name,flag,s["different_offsets"][:10]))
   if s["exact"]:
    print("NEW_EXACT",name,flag,path,flush=True);raise SystemExit(0)
  except ValueError as e:print("REJECT",name,flag,str(e)[:100],flush=True)
 print("DONE",name,flush=True)
print("BEST")
for r in sorted(results,reverse=True)[:35]:print(r,flush=True)

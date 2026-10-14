#!/usr/bin/env python3
"""Verify original VUC TranslateFragList<MLGameString> index lifetime.

Disassembly at 0x0806e5b0 stores the source-list induction update after the
result Add() body (0x806e6a5), whereas an earlier provisional C++03 candidate
increments before allocating the IR fragment. Probe this real, observable
source-level ordering and plausible loop induction ownership in historical GCC.
"""
from pathlib import Path
import importlib.util,itertools,json,subprocess,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location("m",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=json.loads((R/"vuc/config/translate_frag_list_string_oct14_probe.json").read_text())
binary,elf=m.decomp.load();sec=next(x for x in elf["sections"] if x["name"]==".text")
address=int(cfg["address"],0)
original=binary[sec["offset"]+address-sec["address"]:sec["offset"]+address-sec["address"]+306]
source=(R/"vuc/probes/translate_frag_list_string_partial_oct14.cpp").read_text()
source=source.replace('"../src/MLGameString.h"','"'+str(R/'vuc/src/MLGameString.h')+'"')
begin=source.index("    ASTNodeList<IR::Fragment_Data *> output;\n    int i = 0;")
end=source.index("    return output;",begin)
header=source[:begin];footer=source[end:]
loop_condition={
  "plain":"i < input.length",
  "likely":"__builtin_expect((i < input.length),1)",
  "unlikely":"__builtin_expect((i < input.length),0)",
}
operations={
  "reference":"const MLGameString &name = input.entries[index];\n        IR::Fragment_Data *fragment = new IR::Fragment_Data(name);",
  "direct":"IR::Fragment_Data *fragment = new IR::Fragment_Data(input.entries[index]);",
}
index_modes={
  "after-add":("int index = i;","output.Add(fragment);\n        ++i;"),
  "before-add":("int index = i;","++i;\n        output.Add(fragment);"),
  "before-new":("int index = i;\n        ++i;","output.Add(fragment);"),
  "postinc":("int index = i++;","output.Add(fragment);"),
  "indexed-post":("int index = i;","output.Add(fragment);\n        i = index + 1;"),
  "advance-after-alloc":("int index = i;","output.Add(fragment);\n        i = i + 1;"),
}
flags={
  "j1":["-O2","-frename-registers","-falign-jumps=1"],
  "j4":["-O2","-frename-registers","-falign-jumps=4"],
  "j8":["-O2","-frename-registers","-falign-jumps=8"],
  "j4-nr":["-O2","-frename-registers","-falign-jumps=4","-fno-reorder-blocks"],
}
t=Path("/tmp/vuc-binutils215/string-fraglist-index-lifetime-next-oct14");t.mkdir(exist_ok=True,parents=True)
rows=[];sizes=collections.Counter();best=None;best_any=None
for (im,(before,after)),(op,expression),(cname,condition),flag in itertools.product(
 index_modes.items(),operations.items(),loop_condition.items(),flags):
 name=f"{im}-{op}-{cname}-{flag}"
 src=t/(name+".cpp");obj=t/(name+".o")
 piece=('    ASTNodeList<IR::Fragment_Data *> output;\n'
        '    int i=0;\n'
        '    while ('+condition+') {\n'
        '        '+before+'\n'
        '        '+expression+'\n'
        '        '+after+'\n'
        '    }\n')
 src.write_text(header+piece+footer)
 cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*flags[flag],'-c',str(src),'-o',str(obj)],capture_output=True,text=True)
 if cp.returncode:
  rows.append(dict(name=name,error=cp.stderr[-160:]));continue
 try:code,rel=m.prepare_named_function(obj.read_bytes(),cfg["function"],cfg["relocations"],address)
 except ValueError as e:
  rows.append(dict(name=name,error=str(e)));continue
 overlap=sum(a==b for a,b in zip(code,original))
 prefix=next((i for i,(a,b) in enumerate(zip(code,original)) if a!=b),min(len(code),len(original)))
 row=dict(name=name,length=len(code),relocations=rel,overlap=overlap,prefix=prefix)
 rows.append(row);sizes[len(code)]+=1
 if rel==6 and (best_any is None or (prefix,overlap)>(best_any['prefix'],best_any['overlap'])):best_any=row
 if len(code)==306 and rel==6 and (best is None or (overlap,prefix)>(best['overlap'],best['prefix'])):
  best=row
  (t/'best.cpp').write_text(src.read_text());(t/'best.o').write_bytes(obj.read_bytes())
(t/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
print(json.dumps(dict(attempted=len(rows),sizes=dict(sizes),best_original_length=best,best_prefix_any_length=best_any,top=sorted((r for r in rows if 'overlap' in r),key=lambda r:(r['overlap'],r['prefix']),reverse=True)[:12]),indent=2))

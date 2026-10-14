#!/usr/bin/env python3
"""Bounded authentic 2004 C++ interning source/lifetime experiments.

All outputs stay under /tmp in the original Linux VM; only exact-length
historical GCC objects are scored against unmodified original VUC ELF.
"""
from pathlib import Path
import itertools
import importlib.util
import subprocess
ROOT=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/string-add-source-batch-oct14")
T.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location("matcher",ROOT/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)
raw=(ROOT/"vuc/probes/string_table_add_source_oct14.cpp").read_text()
node="""        Node(const K &k, const V &v) {
            hash = H().Hash(k);
            key = k;
            value = v;
        }"""
nodes={
"hash-key-value":node,
"key-hash-value":node.replace("            hash = H().Hash(k);\n            key = k;","            key = k;\n            hash = H().Hash(k);"),
"hash-value-key":node.replace("            key = k;\n            value = v;","            value = v;\n            key = k;"),
"ctor-initializers":"""        Node(const K &k, const V &v) : key(k), hash(H().Hash(k)), value(v) {}""",
"ctor-initializers-next":"""        Node(const K &k, const V &v) : key(k), hash(H().Hash(k)), value(v), next(0) {}""",
"hash-key-value-next":node.replace("            value = v;","            value = v;\n            next = 0;"),
"hash-local":node.replace("            hash = H().Hash(k);","            unsigned h = H().Hash(k);\n            hash = h;"),
}
ctor_src="""    MLGameStringData(const char *input, bool temp) {
        temporary = temp;
        const char *value = input;
        length = strlen(value);
        hash = MLHashFun<const char *>().Hash(value);
        if (temp) text = value;
        else {
            char *copy = new char[length + 1];
            strcpy(copy,value);
            text = copy;
        }
    }"""
data_ctors={
"as-is":ctor_src,
"hash-before-length":ctor_src.replace("        length = strlen(value);\n        hash = MLHashFun<const char *>().Hash(value);","        hash = MLHashFun<const char *>().Hash(value);\n        length = strlen(value);"),
"no-local":ctor_src.replace("        const char *value = input;\n","").replace("value","input"),
"local-after-length":ctor_src.replace("        const char *value = input;\n        length = strlen(value);","        length = strlen(input);\n        const char *value = input;"),
"temp-write-later":ctor_src.replace("        temporary = temp;\n","").replace("        if (temp) text = value;","        temporary = temp;\n        if (temp) text = value;"),
"strcpy-result":ctor_src.replace("            strcpy(copy,value);\n            text = copy;","            text = strcpy(copy,value);"),
}
func_src=raw[raw.index("MLGameStringData *MLGameStringTable::Add("):]
funcs={
"normal":func_src,
"new-block":func_src.replace("    MLGameStringData *data = new MLGameStringData(text, temporary);","    MLGameStringData *data;\n    data = new MLGameStringData(text, temporary);"),
"poststore":func_src.replace("    unsigned index = node->hash % 513;\n    node->next = heads[index];\n    heads[index] = node;",
 "    const unsigned hash = node->hash;\n    unsigned index = hash % 513;\n    node->next = heads[index];\n    heads[index] = node;"),
"slotpointer":func_src.replace("    unsigned index = node->hash % 513;\n    node->next = heads[index];\n    heads[index] = node;",
 "    Table::Node **bucket = &heads[node->hash % 513];\n    node->next = *bucket;\n    *bucket = node;"),
"slotpointer-old":func_src.replace("    unsigned index = node->hash % 513;\n    node->next = heads[index];\n    heads[index] = node;",
 "    Table::Node **bucket = &heads[node->hash % 513];\n    Table::Node *old = *bucket;\n    node->next = old;\n    *bucket = node;"),
}
flags={
"r":["-O2","-frename-registers"],
"rj1":["-O2","-frename-registers","-falign-jumps=1"],
"rj2":["-O2","-frename-registers","-falign-jumps=2"],
"rj4":["-O2","-frename-registers","-falign-jumps=4"],
"rj8":["-O2","-frename-registers","-falign-jumps=8"],
"rnrj1":["-O2","-frename-registers","-fno-reorder-blocks","-falign-jumps=1"],
"nr":["-O2"],
"nrj4":["-O2","-falign-jumps=4"],
}
rows=[]
# limit variants to source structures already demonstrated by VUC disassembly
for (n,node_code),(d,data_code),(f,func_code) in itertools.product(nodes.items(),data_ctors.items(),funcs.items()):
 key="-".join((n,d,f))
 s=raw.replace(node,node_code).replace(ctor_src,data_code).replace(func_src,func_code)
 if s==raw and key!="hash-key-value-as-is-normal":pass
 src=T/(key+".cpp");src.write_text(s)
 for fl,opts in flags.items():
  obj=T/(key+"-"+fl+".o")
  build=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*opts,"-c",str(src),"-o",str(obj)],capture_output=True,text=True)
  if build.returncode:
   print("COMPILER_ERROR",key,fl,build.stderr[-90:],flush=True);continue
  record=next((x for x in subprocess.check_output(["nm","-S",str(obj)],text=True).splitlines() if x.endswith("_ZN17MLGameStringTable3AddEPKcb")),"")
  size=int(record.split()[1],16) if record else -1
  if size==263:
   try:
    score=m.run(obj,ROOT/"vuc/config/string_table_add_oct14_probe.json")
    rows.append((score["exact_bytes"],size,key,fl,score["different_offsets"][:12]))
    if score["exact"]:
      print("EXACT",key,fl,str(src),flush=True)
      raise SystemExit(0)
   except ValueError as e:
    rows.append((0,size,key,fl,"REJECT:"+str(e)[:90]))
  else:rows.append((0,size,key,fl,"length"))
 print("CHECKED",key,flush=True)
print("SUMMARY",len(rows),"original_length",sum(x[1]==263 for x in rows),flush=True)
for x in sorted(rows,reverse=True)[:38]:
 print("RESULT",x,flush=True)

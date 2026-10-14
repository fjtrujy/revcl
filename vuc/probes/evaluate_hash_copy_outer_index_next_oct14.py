#!/usr/bin/env python3
"""Probe genuine outer-bucket-index and nested-loop ownership for VUC hash copy.

Starting point is the already independently verified original-length 80/166
experimental Node four-argument copy constructor. All authentic call+EH
relocations must resolve and final size must equal original 166 bytes.
"""
from pathlib import Path
import itertools,importlib.util,json,subprocess,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location("m",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=json.loads((R/"vuc/config/hash_copy_node_args_partial_oct14_probe.json").read_text())
raw,elf=m.decomp.load();s=next(x for x in elf["sections"] if x["name"]==".text")
address=int(cfg["address"],0)
original=raw[s["offset"]+address-s["address"]:s["offset"]+address-s["address"]+166]
base=(R/cfg["source"]).read_text().replace('#include "MLHashTableCopyNodePartial.h"','#include "'+str(R/"vuc/probes/MLHashTableCopyNodePartial.h")+'"').replace('#include "../src/MLHashFunGameString.h"','#include "'+str(R/"vuc/src/MLHashFunGameString.h")+'"')
outer="for (int i = 0; i < Buckets; ++i)"
inner="""        Node *node = other.heads[i];
        while (node) {
            Node *next = node->next;
            Node *copy = new Node(node->key, node->hash, node->value, node->next);
            copy->next = 0;
            *tail = copy;
            tail = &copy->next;
            node = next;
        }"""
assert outer in base and inner in base
indices={
 "int":outer,
 "unsigned":"for (unsigned i = 0; i < Buckets; ++i)",
 "size_t":"for (std::size_t i = 0; i < Buckets; ++i)",
 "volatile-int":"for (volatile int i = 0; i < Buckets; ++i)",
 "volatile-unsigned":"for (volatile unsigned i = 0; i < Buckets; ++i)",
 "int-post":"for (int i = 0; i < Buckets; i++)",
 "unsigned-post":"for (unsigned i = 0; i < Buckets; i++)",
 "register":"for (register int i = 0; i < Buckets; ++i)",
 "down":"for (int i = Buckets; i-- > 0; )",
}
inners={
 "base":inner,
 "tail-before-zero":inner.replace("            copy->next = 0;\n            *tail = copy;", "            *tail = copy;\n            copy->next = 0;"),
 "advance-before-tail":inner.replace("            tail = &copy->next;\n            node = next;","            node = next;\n            tail = &copy->next;"),
 "next-after-copy":inner.replace("            Node *next = node->next;\n","").replace("            node = next;","            node = node->next;"),
 "copy-next-before":inner.replace("            Node *next = node->next;\n","").replace("new Node(node->key, node->hash, node->value, node->next)","new Node(node->key, node->hash, node->value, node->next)").replace("            node = next;","            node = node->next;"),
 "do-while":inner.replace("        while (node) {","        if (node) do {").replace("            node = next;\n        }","            node = next;\n        } while (node);"),
}
flags={
 "saved":["-O2","-frename-registers","-falign-jumps=1"],
 "j4":["-O2","-frename-registers","-falign-jumps=4"],
 "j8":["-O2","-frename-registers","-falign-jumps=8"],
 "no-ren":["-O2","-falign-jumps=1"],
}
T=Path("/tmp/vuc-binutils215/hash-copy-outer-index-next-oct14");T.mkdir(parents=True,exist_ok=True)
rows=[];sizes=collections.Counter();best=None
for (idx,idxexpr),(ins,body),(flag,opts) in itertools.product(indices.items(),inners.items(),flags.items()):
 tag=f"{idx}-{ins}-{flag}"
 src=base.replace(outer,idxexpr).replace(inner,body)
 cpp=T/(tag+".cpp");obj=T/(tag+".o");cpp.write_text(src)
 cp=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*opts,"-c",str(cpp),"-o",str(obj)],capture_output=True,text=True)
 if cp.returncode:rows.append(dict(tag=tag,error=cp.stderr[-150:]));continue
 try:code,rel=m.prepare_named_function(obj.read_bytes(),cfg["function"],cfg["relocations"],address)
 except ValueError as e:rows.append(dict(tag=tag,error=str(e)));continue
 score=sum(a==b for a,b in zip(code,original))
 prefix=next((i for i,(a,b) in enumerate(zip(code,original)) if a!=b),min(len(code),len(original)))
 row=dict(tag=tag,length=len(code),relocations=rel,overlap=score,prefix=prefix)
 rows.append(row);sizes[len(code)]+=1
 if len(code)==166 and rel==4 and (best is None or (score,prefix)>(best["overlap"],best["prefix"])):
  best=row;(T/"best.cpp").write_text(src);(T/"best.flags").write_text(" ".join(opts)+"\n");(T/"best.o").write_bytes(obj.read_bytes())
(T/"results.json").write_text(json.dumps(rows,indent=2)+"\n")
print(json.dumps(dict(attempted=len(rows),sizes=dict(sizes),best=best,top=sorted((r for r in rows if "overlap" in r),key=lambda r:(r["overlap"],r["prefix"]),reverse=True)[:12]),indent=2))

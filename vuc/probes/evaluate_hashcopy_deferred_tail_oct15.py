#!/usr/bin/env python3
"""VUC hash copy reconstruction: defer tail owner until bucket non-empty.

Original 166-byte constructor sets destination head, probes source bucket,
and only then computes/stores the destination-tail address. Test genuine
C++03 empty-bucket guard and while/for/do iteration with authentic Node copy
and field ownership; no fabricated stack pads or handwritten x86.
"""
from pathlib import Path
import importlib.util,itertools,json,subprocess,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location("m",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=json.loads((R/"vuc/config/hash_copy_node_args_partial_oct14_probe.json").read_text())
raw,elf=m.decomp.load();sec=next(s for s in elf["sections"] if s["name"]==".text")
addr=int(cfg["address"],0);original=raw[sec["offset"]+addr-sec["address"]:sec["offset"]+addr-sec["address"]+166]
source=(R/"vuc/probes/hash_copy_node_args_partial_oct14.cpp").read_text()
head=(R/"vuc/probes/MLHashTableCopyNodePartial.h").read_text()
start=source.index("    for (int i = 0; i < Buckets; ++i) {")
end=source.index("\n}\ntemplate MLHashTable",start)
preamble=source[:start];endpart=source[end:]
constructors={
 "four":("Node(const Key &k, unsigned h, Value v, Node *n) : key(k), hash(h), value(v), next(n) {}","new Node(node->key, node->hash, node->value, node->next)",""),
 "three":("Node(const Key &k, unsigned h, Value v) : key(k), hash(h), value(v) {}","new Node(node->key, node->hash, node->value)",""),
 "two":("Node(const Key &k, unsigned h) : key(k), hash(h) {}","new Node(node->key, node->hash)","            copy->value = node->value;\n"),
 "key":("Node(const Key &k) : key(k) {}","new Node(node->key)","            copy->hash = node->hash;\n            copy->value = node->value;\n"),
}
oldctor="Node(const Key &k, unsigned h, Value v, Node *n) : key(k), hash(h), value(v), next(n) {}"
orders={
 "tail-after-node":"""        heads[i] = 0;
        Node *node = other.heads[i];
        if (!node) continue;
        Node **tail = &heads[i];
""",
 "tail-after-positive":"""        heads[i] = 0;
        Node *node = other.heads[i];
        if (node) {
            Node **tail = &heads[i];
""",
 "tail-after-loop":"""        heads[i] = 0;
        Node *node = other.heads[i];
        if (node != 0) {
            Node **tail = &heads[i];
""",
 "baseline":"""        heads[i] = 0;
        Node **tail = &heads[i];
        Node *node = other.heads[i];
""",
 "head-index":"""        Node *node = other.heads[i];
        heads[i] = 0;
        if (!node) continue;
        Node **tail = &heads[i];
""",
}
loopmodes={
 "while":("        while (node) {","            node = next;\n        }"),
 "do":("        do {","            node = next;\n        } while (node);"),
 "for":("        for (; node; ) {","            node = next;\n        }"),
}
orderstores={
 "normal":"""            copy->next = 0;
            *tail = copy;
            tail = &copy->next;
""",
 "tail-first":"""            *tail = copy;
            copy->next = 0;
            tail = &copy->next;
""",
 "tail-before-zero":"""            *tail = copy;
            tail = &copy->next;
            copy->next = 0;
""",
 "zero-after-payload":"""            copy->next = 0;
            tail = &copy->next;
            *tail = copy;
""",
}
flags={
 "j1":["-O2","-frename-registers","-falign-jumps=1"],
 "j4":["-O2","-frename-registers","-falign-jumps=4"],
 "plain":["-O2","-falign-jumps=1"],
 "j8":["-O2","-frename-registers","-falign-jumps=8"],
}
T=Path("/tmp/vuc-binutils215/hash-copy-deferred-tail-oct15");T.mkdir(parents=True,exist_ok=True)
rows=[];sizes=collections.Counter();best=None
for (c,(ctor,call,extrafields)),(o,owner),(mode,(opening,ending)),(store,stores),(f,flags_) in itertools.product(constructors.items(),orders.items(),loopmodes.items(),orderstores.items(),flags.items()):
 # zero-after-payload changes pointer chain semantics: exclude as not equivalent
 if store=="zero-after-payload":continue
 tag=f"{c}-{o}-{mode}-{store}-{f}"
 before="    for (int i = 0; i < Buckets; ++i) {\n"+owner+opening+"\n"
 inner=(
 "            Node *next = node->next;\n"
 "            Node *copy = "+call+";\n"
 +extrafields+stores+ending
 )
 suffix=("\n        }" if o in ("tail-after-positive","tail-after-loop") else "")+"\n    }"
 code=preamble+before+inner+suffix+endpart
 h=T/(tag+".h");cpp=T/(tag+".cpp");obj=T/(tag+".o")
 h.write_text(head.replace(oldctor,ctor))
 cpp.write_text(code.replace('#include "MLHashTableCopyNodePartial.h"','#include "'+str(h)+'"').replace('#include "../src/MLHashFunGameString.h"','#include "'+str(R/"vuc/src/MLHashFunGameString.h")+'"'))
 cc=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*flags_,"-c",str(cpp),"-o",str(obj)],capture_output=True,text=True)
 if cc.returncode:rows.append(dict(tag=tag,error=cc.stderr[-140:]));continue
 try:machine,rels=m.prepare_named_function(obj.read_bytes(),cfg["function"],cfg["relocations"],addr)
 except ValueError as e:rows.append(dict(tag=tag,error=str(e)));continue
 score=sum(a==b for a,b in zip(machine,original))
 prefix=next((i for i,(a,b) in enumerate(zip(machine,original)) if a!=b),min(len(machine),len(original)))
 rec=dict(tag=tag,length=len(machine),relocations=rels,match=score,prefix=prefix)
 rows.append(rec);sizes[len(machine)]+=1
 if len(machine)==166 and rels==4 and (best is None or (score,prefix)>(best["match"],best["prefix"])):
  best=rec;(T/"best.h").write_text(h.read_text());(T/"best.cpp").write_text(cpp.read_text());(T/"best.flags").write_text(" ".join(flags_)+"\n");(T/"best.o").write_bytes(obj.read_bytes())
(T/"results.json").write_text(json.dumps(rows,indent=2)+"\n")
print(json.dumps(dict(attempted=len(rows),sizes=dict(sizes),best=best,top=sorted((r for r in rows if "match" in r),key=lambda r:(r["match"],r["prefix"]),reverse=True)[:14]),indent=2))

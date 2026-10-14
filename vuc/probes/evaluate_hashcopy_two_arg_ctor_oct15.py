#!/usr/bin/env python3
"""VUC hash-copy 166-byte C++03 constructor field-ownership hypotheses.

Original ELF performs new Node(16), MLGameString copy, hash assignment,
tail connection, payload assignment and next=0. Test plausible constructor
argument counts and post-copy member assignments, checking the exact
original 166-byte size and all four legitimate exception relocations.
"""
from pathlib import Path
import itertools,importlib.util,json,subprocess,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location("m",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=json.loads((R/"vuc/config/hash_copy_node_args_partial_oct14_probe.json").read_text())
raw,elf=m.decomp.load();s=next(x for x in elf["sections"] if x["name"]==".text")
addr=int(cfg["address"],0);ref=raw[s["offset"]+addr-s["address"]:s["offset"]+addr-s["address"]+166]
header=(R/"vuc/probes/MLHashTableCopyNodePartial.h").read_text()
source=(R/"vuc/probes/hash_copy_node_args_partial_oct14.cpp").read_text()
headold="Node(const Key &k, unsigned h, Value v, Node *n) : key(k), hash(h), value(v), next(n) {}"
callold="Node *copy = new Node(node->key, node->hash, node->value, node->next);"
assert headold in header and callold in source
forms={
 "key":"Node(const Key &k) : key(k) {}",
 "keyhash":"Node(const Key &k, unsigned h) : key(k), hash(h) {}",
 "keyhash-value":"Node(const Key &k, unsigned h, Value v) : key(k), hash(h), value(v) {}",
 "keyhash-next":"Node(const Key &k, unsigned h, Node *n) : key(k), hash(h), next(n) {}",
 "keyvalue":"Node(const Key &k, Value v) : key(k), value(v) {}",
 "keyvalue-next":"Node(const Key &k, Value v, Node *n) : key(k), value(v), next(n) {}",
 "keyvaluehash":"Node(const Key &k, Value v, unsigned h) : key(k), hash(h), value(v) {}",
 "keyhashvaluenext":headold,
 "key-valueassign":"Node(const Key &k, unsigned h) : key(k) { hash=h; }",
 "keyhash-nextzero":"Node(const Key &k, unsigned h) : key(k), hash(h), next(0) {}",
 "keyhash-valuezero":"Node(const Key &k, unsigned h) : key(k), hash(h), value() {}",
 "keyhash-bothzero":"Node(const Key &k, unsigned h) : key(k), hash(h), value(), next(0) {}",
}
fields={
 "key":("node->key",("hash","value")),
 "keyhash":("node->key, node->hash",("value",)),
 "keyhash-value":("node->key, node->hash, node->value",()),
 "keyhash-next":("node->key, node->hash, node->next",("value",)),
 "keyvalue":("node->key, node->value",("hash",)),
 "keyvalue-next":("node->key, node->value, node->next",("hash",)),
 "keyvaluehash":("node->key, node->value, node->hash",()),
 "keyhashvaluenext":("node->key, node->hash, node->value, node->next",()),
 "key-valueassign":("node->key, node->hash",("value",)),
 "keyhash-nextzero":("node->key, node->hash",("value",)),
 "keyhash-valuezero":("node->key, node->hash",("value",)),
 "keyhash-bothzero":("node->key, node->hash",("value",)),
}
styles={
 "save-next":"        Node *next = node->next;",
 "save-next-pointer":"        Node *const next = node->next;",
 "save-next-ref":"        Node *const &next = node->next;",
}
placements={
 "after-copy":"            copy->next = 0;\n            *tail = copy;",
 "before-tail":"            *tail = copy;\n            copy->next = 0;",
 "post-tail":"            *tail = copy;\n            tail = &copy->next;\n            copy->next = 0;",
}
flags={
 "j1":["-O2","-frename-registers","-falign-jumps=1"],
 "j4":["-O2","-frename-registers","-falign-jumps=4"],
 "plain":["-O2","-falign-jumps=1"],
}
T=Path("/tmp/vuc-binutils215/hash-copy-two-arg-ctor-batch-oct15");T.mkdir(exist_ok=True,parents=True)
rows=[];lengths=collections.Counter();best=None
for (name,hctor),(sty,sav),(plc,placement),(fl,opts) in itertools.product(forms.items(),styles.items(),placements.items(),flags.items()):
 tag=f"{name}-{sty}-{plc}-{fl}"
 args,extras=fields[name]
 cpp_text=source.replace(callold,f"Node *copy = new Node({args});")
 cpp_text=cpp_text.replace("            Node *next = node->next;",sav)
 post="\n".join(f"            copy->{x} = node->{x};" for x in extras)
 if post:post+="\n"
 cpp_text=cpp_text.replace("            copy->next = 0;\n            *tail = copy;",post+placement)
 if plc=="post-tail":
  cpp_text=cpp_text.replace("            tail = &copy->next;\n            node = next;", "            node = next;")
 h=T/(tag+".h");cpp=T/(tag+".cpp");obj=T/(tag+".o")
 h.write_text(header.replace(headold,hctor))
 cpp.write_text(cpp_text.replace('#include "MLHashTableCopyNodePartial.h"','#include "'+str(h)+'"').replace('#include "../src/MLHashFunGameString.h"','#include "'+str(R/"vuc/src/MLHashFunGameString.h")+'"'))
 result=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*opts,"-c",str(cpp),"-o",str(obj)],capture_output=True,text=True)
 if result.returncode:rows.append(dict(tag=tag,error=result.stderr[-120:]));continue
 try:machine,rels=m.prepare_named_function(obj.read_bytes(),cfg["function"],cfg["relocations"],addr)
 except ValueError as e:rows.append(dict(tag=tag,error=str(e)));continue
 equal=sum(a==b for a,b in zip(ref,machine))
 prefix=next((i for i,(a,b) in enumerate(zip(ref,machine)) if a!=b),min(len(ref),len(machine)))
 rec=dict(tag=tag,length=len(machine),relocations=rels,match=equal,prefix=prefix)
 rows.append(rec);lengths[len(machine)]+=1
 if len(machine)==166 and rels==4 and (best is None or (equal,prefix)>(best["match"],best["prefix"])):
  best=rec;(T/"best.h").write_text(h.read_text());(T/"best.cpp").write_text(cpp.read_text());(T/"best.o").write_bytes(obj.read_bytes());(T/"best.flags").write_text(" ".join(opts)+"\n")
(T/"results.json").write_text(json.dumps(rows,indent=2)+"\n")
print(json.dumps(dict(attempted=len(rows),sizes=dict(lengths),best=best,top=sorted((x for x in rows if "match" in x),key=lambda x:(x["match"],x["prefix"]),reverse=True)[:12]),indent=2))

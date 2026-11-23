#!/usr/bin/env python3
"""Genuine source-level exception/branch owner controls for VUC SymbolTable::Enter.

Under canonical recovered GCC 3.3, normal SymbolTable::Enter is 180 bytes
but branch-target-aligned j8 version is 186 vs original 185, the sole extra
byte being a natural NOP in front of its EH landing pad. Test source-level
C++03 empty-scope, Node allocation and hash-table bucket-owner forms
that may change GCC branch probability/EH placement without fabricated
source padding, hard register constraints, or compiler output patches.

No source/headers in vuc/src are modified. All exact siblings are checked
when an original-sized candidate with authenticated relocations is found.
"""
from pathlib import Path
import json, importlib.util, subprocess
V=Path(__file__).resolve().parents[1]
O=Path("/tmp/vuc-symbol-enter-source-eh-20261123");O.mkdir(exist_ok=True)
src=(V/"src/SymbolTableUnit.cpp").read_text()
for file in ("MLGameString.h","MLHashTable.h","MLHashFunGameString.h"):
    src=src.replace('#include "'+file+'"', '#include "'+str(V/"src"/file)+'"')
start=src.index("void SymbolTable::Enter(")
end=src.index("SymbolTableEntry *SymbolTable::Find(",start)
head=src[:start]
body=src[start:end]
tail=src[end:]
guard="    if (scopes_.empty()) return;"
scope="    SymbolScope &scope = scopes_[scopes_.size() - 1];"
hasher="    MLHashFun<MLGameString> hasher;"
node="    SymbolScope::Node *node = new SymbolScope::Node(name, hasher.Hash(name), entry);"
bucket="    unsigned bucket = node->hash % 513;"
links="    node->next = scope.heads[bucket];\n    scope.heads[bucket] = node;"
for n in (guard,scope,hasher,node,bucket,links):
    assert n in body,n
variants={"baseline":body}
def swap(n, old, new):
  assert old in body
  variants[n]=body.replace(old,new,1)
for n,t in {
"empty-branch-predict-true":"    if (__builtin_expect(scopes_.empty(),1)) return;",
"empty-branch-predict-false":"    if (__builtin_expect(scopes_.empty(),0)) return;",
"nonempty-branch-predict-true":"    if (__builtin_expect(!scopes_.empty(),1)==0) return;",
"nonempty-branch-predict-false":"    if (__builtin_expect(!scopes_.empty(),0)==0) return;",
"size-zero":"    if (scopes_.size() == 0) return;",
"size-nonzero":"    if (!(scopes_.size() != 0)) return;",
"size-less-one":"    if (scopes_.size() < 1) return;",
"begin-equals-end":"    if (scopes_.begin() == scopes_.end()) return;",
"empty-braces":"    if (scopes_.empty()) { return; }",
"empty-excl":"    if (!(!scopes_.empty())) return;",
"empty-unsigned":"    if ((unsigned)scopes_.size() == 0U) return;",
}.items():swap(n,guard,t)
for n,t in {
"scope-back":"    SymbolScope &scope = scopes_.back();",
"scope-end-index":"    SymbolScope &scope = *(scopes_.end() - 1);",
"scope-begin-index":"    SymbolScope &scope = *(scopes_.begin() + (scopes_.size()-1));",
"scope-pointer-index":"    SymbolScope &scope = *(&scopes_[0] + scopes_.size()-1);",
}.items():swap(n,scope,t)
for n,t in {
"ctor-call-hasher-temp":"    SymbolScope::Node *node = new SymbolScope::Node(name, MLHashFun<MLGameString>().Hash(name), entry);",
"ctor-qualified-hasher":"    SymbolScope::Node *node = new SymbolScope::Node(name, hasher.Hash(name), entry);",
}.items():swap(n,node,t)
for n,t in {
"bucket-const":"    const unsigned bucket = node->hash % 513;",
"bucket-int":"    int bucket = node->hash % 513;",
"bucket-const-int":"    const int bucket = node->hash % 513;",
"bucket-remainder":"    unsigned bucket = node->hash - (node->hash/513)*513;",
"bucket-mod-static":"    static const unsigned bucketCount = 513;\n    unsigned bucket = node->hash % bucketCount;",
"bucket-sized":"    unsigned bucket = node->hash % (sizeof(scope.heads)/sizeof(scope.heads[0]));",
}.items():swap(n,bucket,t)
for n,t in {
"link-old-local":"    SymbolScope::Node *old = scope.heads[bucket];\n    node->next = old;\n    scope.heads[bucket] = node;",
"link-pointer":"    SymbolScope::Node **head = &scope.heads[bucket];\n    node->next = *head;\n    *head = node;",
"link-head-typed":"    SymbolScope::Node *&head = scope.heads[bucket];\n    node->next = head;\n    head = node;",
"link-prehead":"    SymbolScope::Node *old = scope.heads[bucket];\n    scope.heads[bucket] = node;\n    node->next = old;",
"link-conditional-expression":"    SymbolScope::Node **head = &scope.heads[bucket];\n    (node->next = *head, *head = node);",
"link-reverse-expression":"    SymbolScope::Node *old = scope.heads[bucket];\n    node->next = old;\n    *(scope.heads+bucket) = node;",
"link-index-scopes":"    node->next = scopes_[scopes_.size()-1].heads[bucket];\n    scopes_[scopes_.size()-1].heads[bucket] = node;",
}.items():swap(n,links,t)
swap("hasher-before-guard",hasher+ "\n", "")
variants["hasher-before-guard"]=body.replace(hasher+"\n","").replace(guard,hasher+"\n"+guard,1)
variants["explicit-success-return"]=body.replace(links,links+"\n    return;")
variants["node-const-pointer"]=body.replace(node,
            "    SymbolScope::Node *const node = new SymbolScope::Node(name, hasher.Hash(name), entry);")
variants["node-ctor-helper"]=body.replace(node,
            "    SymbolScope::Node *node = makeNode(name, hasher.Hash(name), entry);")
helper="static inline SymbolScope::Node *makeNode(const MLGameString& name, unsigned hash, SymbolTableEntry *entry) { return new SymbolScope::Node(name,hash,entry); }\n"
spec=importlib.util.spec_from_file_location("m",V/"scripts/match_probe.py")
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
cfgs={
 "enter":"symboltable_enter_oct13_probe.json",
 "find":"symboltable_find_probe.json",
 "begin":"symbol_begin_scope_probe.json",
 "end":"symboltable_end_exact_probe.json",
 "scope":"scope_vector_insert_partial_oct15i.json"
}
rows=[]
for label,source in variants.items():
    prefix=head+(helper if label=="node-ctor-helper" else "")
    path=O/(label+".cpp");path.write_text(prefix+source+tail)
    for snap in ("jan25","dec06"):
      obj=O/(label+"-"+snap+".o")
      cp=subprocess.run(["/tmp/vuc-binutils215/g++-"+snap+"-as215",
             "-O2","-frename-registers","-falign-jumps=8","-falign-labels=1",
             "-falign-loops=1","-c",str(path),"-o",str(obj)],capture_output=True,text=True)
      row={"variant":label,"compiler":snap}
      if cp.returncode:row["error"]=cp.stderr[-300:]
      else:
        try:
          v=m.run(obj,V/"config"/cfgs["enter"])
          row["enter"]={x:v[x] for x in ("length","exact_bytes","different_offsets","relocations","exact")}
        except ValueError as ex:row["enter"]={"unverified":str(ex)}
        if row["enter"].get("length")==185:
          for sibling in ("find","begin","end","scope"):
            try:
              s=m.run(obj,V/"config"/cfgs[sibling])
              row[sibling]=[s["exact_bytes"],s["length"]]
            except ValueError as e:row[sibling]=str(e)
      rows.append(row);print(json.dumps(row,sort_keys=True),flush=True)
(O/"results.json").write_text(json.dumps(rows,indent=2)+"\n")

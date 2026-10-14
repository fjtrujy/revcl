#!/usr/bin/env python3
"""Transfer the byte-exact C++03 clear() throw() declaration to scope iterators.

Only original-length historical compiler functions with all six expected
relocations are candidates; all experimental files remain in Lima scratch.
"""
from pathlib import Path
import importlib.util,itertools,subprocess,json,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location("match",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=json.loads((R/"vuc/config/scope_iterator_copy_oct12_probe.json").read_text())
raw,target=m.decomp.load()
section=next(s for s in target["sections"] if s["name"]==".text")
address=int(cfg["address"],0)
original=raw[section["offset"]+address-section["address"]:section["offset"]+address-section["address"]+149]
include='''#include <vector>
#include <new>
#include <bits/stl_uninitialized.h>
#include "'''+str(R/"vuc/src/MLHashFunGameString.h")+'''"
#include "'''+str(R/"vuc/src/ScopeCopyTable.h")+'''"
struct SymbolTableEntry;
typedef MLHashTable<MLGameString, SymbolTableEntry*, MLHashFun<MLGameString>, 513> Scope;
typedef std::vector<Scope> ScopeVector;
typedef ScopeVector::iterator Iter;
namespace std {
template <> Iter __uninitialized_copy_aux<Iter,Iter>(Iter first,Iter last,Iter result,__false_type) {
    Iter current=result;
    try {
'''
tail='''        return current;
    } catch (...) {
{cleanup}
        throw;
    }
}
}
'''
loops={
"for":'''        for (; first!=last; ++first, ++current) {
            if (&*current) ::new((void*)&*current) Scope(*first);
        }
''',
"while":'''        while (first!=last) {
            if (&*current) ::new((void*)&*current) Scope(*first);
            ++first; ++current;
        }
''',
"post":'''        while (first!=last) {
            if (&*current) ::new((void*)&*current) Scope(*first++);
            ++current;
        }
''',
"direct":'''        for (; first!=last; ++first, ++current)
            ::new((void*)&*current) Scope(*first);
''',
}
cleanups={
"for":'''        for (Iter ptr=result; ptr!=current; ++ptr) ptr->clear();''',
"while":'''        Iter ptr=result;
        while (ptr!=current) { ptr->clear(); ++ptr; }''',
"post":'''        Iter ptr=result;
        while (ptr!=current) { Iter old=ptr++; old->clear(); }''',
"pointer":'''        Scope *ptr=&*result, *end=&*current;
        while (ptr!=end) { ptr->clear(); ++ptr; }''',
"before-call":'''        Iter ptr=result;
        while (ptr!=current) { Iter old=ptr; ++ptr; old->clear(); }''',
}
flags={
"j1":["-O2","-frename-registers","-falign-jumps=1"],
"j4":["-O2","-frename-registers","-falign-jumps=4"],
"j8":["-O2","-frename-registers","-falign-jumps=8"],
"j4-nr":["-O2","-frename-registers","-falign-jumps=4","-fno-reorder-blocks"],
}
out=Path("/tmp/vuc-binutils215/scope-iterator-throw-oct14")
out.mkdir(exist_ok=True,parents=True)
records=[];lengths=collections.Counter();best=None
for (a,loop),(b,cleanup),(f,opts) in itertools.product(loops.items(),cleanups.items(),flags.items()):
 name=a+"-"+b+"-"+f
 cpp=out/(name+".cpp");obj=out/(name+".o")
 cpp.write_text(include+loop+tail.replace('{cleanup}',cleanup))
 run=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*opts,"-c",str(cpp),"-o",str(obj)],capture_output=True,text=True)
 if run.returncode:
  records.append(dict(name=name,error=run.stderr[:150]));continue
 try:code,rels=m.prepare_named_function(obj.read_bytes(),cfg["function"],cfg["relocations"],address)
 except ValueError as exc:
  records.append(dict(name=name,error=str(exc)));continue
 lengths[len(code)]+=1
 overlap=sum(x==y for x,y in zip(code,original))
 prefix=next((i for i,(x,y) in enumerate(zip(code,original)) if x!=y),min(len(code),len(original)))
 row=dict(name=name,length=len(code),relocations=rels,overlap=overlap,prefix=prefix)
 records.append(row)
 if len(code)==149 and rels==6 and (best is None or (overlap,prefix)>(best["overlap"],best["prefix"])):
  best=row
  (out/"best.cpp").write_text(cpp.read_text())
  (out/"best.o").write_bytes(obj.read_bytes())
original_template=(R/"vuc/probes/scope_iterator_clear_throw_oct14.cpp").read_text()
original_template=original_template.replace(
 '"../src/MLHashFunGameString.h"', '"'+str(R/'vuc/src/MLHashFunGameString.h')+'"'
).replace('"../src/ScopeCopyTable.h"','"'+str(R/'vuc/src/ScopeCopyTable.h')+'"')
for f,opts in flags.items():
 name='implicit-template-'+f
 cpp=out/(name+'.cpp');obj=out/(name+'.o');cpp.write_text(original_template)
 run=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if run.returncode:
  records.append(dict(name=name,error=run.stderr[:150]));continue
 try:code,rels=m.prepare_named_function(obj.read_bytes(),cfg['function'],cfg['relocations'],address)
 except ValueError as exc:
  records.append(dict(name=name,error=str(exc)));continue
 lengths[len(code)]+=1
 overlap=sum(x==y for x,y in zip(code,original))
 prefix=next((i for i,(x,y) in enumerate(zip(code,original)) if x!=y),min(len(code),len(original)))
 row=dict(name=name,length=len(code),relocations=rels,overlap=overlap,prefix=prefix)
 records.append(row)
 if len(code)==149 and rels==6 and (best is None or (overlap,prefix)>(best['overlap'],best['prefix'])):
  best=row
  (out/'best.cpp').write_text(cpp.read_text())
  (out/'best.o').write_bytes(obj.read_bytes())
(out/"results.json").write_text(json.dumps(records,indent=2)+"\n")
print(json.dumps(dict(attempted=len(records),generated_lengths=dict(lengths),best_original_length=best,best_overlap=sorted((r for r in records if "overlap" in r),key=lambda r:(r["overlap"],r["prefix"]),reverse=True)[:6],results=str(out/"results.json")),indent=2))

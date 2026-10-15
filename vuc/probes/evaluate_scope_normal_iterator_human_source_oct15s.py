#!/usr/bin/env python3
"""Fresh VUC 149-byte MLHashTable vector iterator-copy C++03 source controls.

Original ELF at 0x0806a840 shows 0x804-byte vector element stepping,
null-checking placement-new, and catch cleanup calling clear().
Try real C++03 template specialization/iterator access idioms rather
than invented machine instructions or broad compiler-flag combinations.
"""
from pathlib import Path
import subprocess,json
R=Path(__file__).resolve().parents[1]
OUT=Path("/tmp/vuc-binutils215/scope-normal-iter-human-oct15s")
OUT.mkdir(exist_ok=True)
base=(R/"probes/ScopeNormalIteratorUninitializedFreshOct15s.cpp").read_text()
base=base.replace('#include "../src/MLHashFunGameString.h"','#include "'+str(R/"src/MLHashFunGameString.h")+'"')
base=base.replace('#include "../src/ScopeCopyTable.h"','#include "'+str(R/"src/ScopeCopyTable.h")+'"')
point=base.index('// Explicit period-C++03')
prefix=base[:point]
forms={
 "implicit":base,
}
for tag,pre,init,loop,clear in (
 ("for-placement","", "ScopeIter current=result;", "for(;first!=last;++first,++current) { if(&*current) ::new((void*)&*current) Scope(*first); }", "for (ScopeIter ptr=result; ptr!=current; ++ptr) ptr->clear();"),
 ("for-allocator","", "ScopeIter current=result;", "for(;first!=last;++first,++current) { if(&*current) new((void*)&*current) Scope(*first); }", "for (ScopeIter ptr=result; ptr!=current; ++ptr) ptr->clear();"),
 ("while-placement","", "ScopeIter current=result;", "while(first!=last) { if(&*current) ::new((void*)&*current) Scope(*first); ++first;++current; }", "for (ScopeIter ptr=result; ptr!=current; ++ptr) ptr->clear();"),
 ("for-pointer","", "ScopeIter current=result;", "for(;first!=last;++first,++current) { Scope *p=&*current; if(p) ::new((void*)p) Scope(*first); }", "ScopeIter ptr=result; while(ptr!=current) {ptr->clear(); ++ptr;}"),
 ("for-pointer-no-if","", "ScopeIter current=result;", "for(;first!=last;++first,++current) { Scope *p=&*current; ::new((void*)p) Scope(*first); }", "ScopeIter ptr=result; while(ptr!=current) {ptr->clear(); ++ptr;}"),
 ("for-dtor","", "ScopeIter current=result;", "for(;first!=last;++first,++current) { Scope *p=&*current; if(p) ::new((void*)p) Scope(*first); }", "for(ScopeIter ptr=result;ptr!=current;++ptr) ptr->~Scope();"),
 ("for-pointer-base","", "ScopeIter current=result;", "for(;first!=last;++first,++current) { Scope *p=current.base(); if(p) ::new((void*)p) Scope(*first); }", "ScopeIter ptr=result; while(ptr!=current) {ptr->clear(); ++ptr;}"),
 ("for-saved-begin","", "ScopeIter current=result;", "for(;first!=last;++first,++current) { Scope *p=&*current; if(p) ::new((void*)p) Scope(*first); }", "for(ScopeIter ptr=result;ptr!=current; ++ptr) (*ptr).clear();"),
 ("post-increment","", "ScopeIter current=result;", "while(first!=last) { Scope *p=&*current; if(p) ::new((void*)p) Scope(*first); first++; current++; }", "for (ScopeIter ptr=result; ptr!=current; ++ptr) ptr->clear();"),
):
 code=prefix+"""
namespace std {
template<> ScopeIter __uninitialized_copy_aux<ScopeIter,ScopeIter>(
    ScopeIter first, ScopeIter last, ScopeIter result, __false_type) {
    """+pre+init+"""
    try {
    """+loop+"""
      return current;
    } catch(...) {
    """+clear+"""
      throw;
    }
}
}
"""
 forms[tag]=code
rows=[]
for tag,code in forms.items():
 cpp=OUT/(tag+".cpp");cpp.write_text(code)
 for compiler in ("jan25","dec06"):
  obj=OUT/(tag+"-"+compiler+".o")
  p=subprocess.run(["/tmp/vuc-binutils215/g++-"+compiler+"-as215","-O2","-frename-registers","-c",str(cpp),"-o",str(obj)],capture_output=True,text=True)
  row={"case":tag,"compiler":compiler}
  if p.returncode:row["error"]=p.stderr[-160:]
  else:
   nm=subprocess.run(["nm","-S","--defined-only",str(obj)],capture_output=True,text=True)
   sym=[x.split() for x in nm.stdout.splitlines() if "uninitialized_copy_aux" in x and len(x.split())>=4]
   row["sizes"]=[int(x[1],16) for x in sym]
  rows.append(row);print(row,flush=True)
(OUT/"results.json").write_text(json.dumps(rows,indent=2)+"\n")

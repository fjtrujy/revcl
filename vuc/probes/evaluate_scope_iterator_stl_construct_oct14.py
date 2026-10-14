#!/usr/bin/env python3
"""Compare genuine 2004 libstdc++ _Construct/_Destroy STL lifetimes and loop hints.

The historical stl_uninitialized.h implements __uninitialized_copy_aux using
_Construct and _Destroy. Previous manual placement-new proposals removed EH;
these preserve historical helper spelling in an explicit specialization.
All generated .o/cpp results stay in Lima scratch until byte-authenticated.
"""
from pathlib import Path
import itertools,importlib.util,subprocess,collections,json
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('match',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
config=json.loads((R/'vuc/config/scope_iterator_copy_oct12_probe.json').read_text())
raw,elf=m.decomp.load();sec=next(s for s in elf['sections'] if s['name']=='.text')
addr=int(config['address'],0);orig=raw[sec['offset']+addr-sec['address']:sec['offset']+addr-sec['address']+149]
t=Path('/tmp/vuc-binutils215/scope-iterator-real-stl-helpers-oct14');t.mkdir(exist_ok=True,parents=True)
head='''#include <vector>
#include <bits/stl_uninitialized.h>
#include "'''+str(R/'vuc/src/MLHashFunGameString.h')+'''"
#include "'''+str(R/'vuc/src/ScopeCopyTable.h')+'''"
struct SymbolTableEntry;
typedef MLHashTable<MLGameString,SymbolTableEntry*,MLHashFun<MLGameString>,513> Scope;
typedef std::vector<Scope> ScopeVector;
typedef ScopeVector::iterator Iter;
namespace std {
template <> Iter __uninitialized_copy_aux<Iter,Iter>(
  Iter first,Iter last,Iter result,__false_type) {
    Iter cur=result;
    try {
'''
tail='''        return cur;
    } catch (...) {
        _Destroy(result,cur);
        __throw_exception_again;
    }
}
}
'''
conditions={
 'plain':'first != last',
 'likely':'__builtin_expect(first != last,1)',
 'unlikely':'__builtin_expect(first != last,0)',
 'negated':'!__builtin_expect(first == last,0)',
}
loops={
 'for':'        for (; {c}; ++first,++cur) _Construct(&*cur,*first);\n',
 'while':'        while ({c}) { _Construct(&*cur,*first); ++first; ++cur; }\n',
 'post':'        while ({c}) { _Construct(&*cur,*first++); ++cur; }\n',
 'do':'        if ({c}) do { _Construct(&*cur,*first); ++first; ++cur; } while ({c});\n',
}
flags={
 'base':['-O2','-frename-registers'],
 'j1':['-O2','-frename-registers','-falign-jumps=1'],
 'j4':['-O2','-frename-registers','-falign-jumps=4'],
 'j8':['-O2','-frename-registers','-falign-jumps=8'],
 'l2':['-O2','-frename-registers','-falign-labels=2'],
 'l4':['-O2','-frename-registers','-falign-labels=4'],
 'nr':['-O2','-frename-registers','-fno-reorder-blocks'],
}
rows=[];sizes=collections.Counter();best=None
for (k,c),(l,loop),(f,opts) in itertools.product(conditions.items(),loops.items(),flags.items()):
 name=f'{k}-{l}-{f}'
 cpp=t/(name+'.cpp');obj=t/(name+'.o')
 cpp.write_text(head+loop.replace('{c}',c)+tail)
 proc=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if proc.returncode:
  rows.append(dict(name=name,error=proc.stderr[-160:]));continue
 try:code,rels=m.prepare_named_function(obj.read_bytes(),config['function'],config['relocations'],addr)
 except ValueError as e:
  rows.append(dict(name=name,error=str(e)));continue
 sizes[len(code)]+=1
 overlap=sum(a==b for a,b in zip(code,orig))
 prefix=next((i for i,(a,b) in enumerate(zip(code,orig)) if a!=b),min(len(code),len(orig)))
 row=dict(name=name,length=len(code),relocations=rels,overlap=overlap,prefix=prefix)
 rows.append(row)
 if len(code)==149 and rels==6 and (best is None or (overlap,prefix)>(best['overlap'],best['prefix'])):
  best=row;(t/'best.cpp').write_text(cpp.read_text());(t/'best.o').write_bytes(obj.read_bytes())
(t/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
print(json.dumps(dict(attempted=len(rows),sizes=dict(sizes),best_original_length=best,top=sorted((x for x in rows if 'overlap' in x),key=lambda x:(x['overlap'],x['prefix']),reverse=True)[:8]),indent=2))

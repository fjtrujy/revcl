#!/usr/bin/env python3
"""Natural recovered Canonize.cpp TU neighbor owner, not flag sweeping.

Co-compile *five newly reconstructed functions* in original function order:
Canonize_TryReduceExpr<unsigned/int>, ReduceBinOp<unsigned/int>, and
ConcatenateBinOp. Recover historical shared template instantiation/codegen
state without modifying accepted code, machine bytes, or link relocations.
"""
from pathlib import Path
import importlib.util,json,subprocess
ROOT=Path(__file__).resolve().parents[2]
R=ROOT/'vuc'; O=Path('/tmp/vuc-binutils215/canonize-real-unit-oct15p')
O.mkdir(exist_ok=True)
canon=(R/'probes/CanonizeTryReduceNewOct15p.cpp').read_text()
reduce=(R/'probes/ReduceBinOpNewOct15o.cpp').read_text()
concat=(R/'probes/ConcatenateBinOpNewOct15o.cpp').read_text()
cbody=canon[canon.index('template<class T>\nbool Canonize_TryReduceExpr'):]
rbody=reduce[reduce.index('template<class T>\nbool ReduceBinOp'):]
sbody=concat[concat.index('MLGameString ConcatenateBinOp'):]
context='''// Partial reconstruction of original Canonize.cpp, original symbol order.
#include "'''+str(R/'src/MLHashTable.h')+'''"
#include "'''+str(R/'src/MLHashFunGameString.h')+'''"
#include <cctype>
#include <cstdlib>
namespace IR {
struct Operator_Data { unsigned vt; unsigned operation; };
struct Expr_Data {
    unsigned vt;
    unsigned kind;
    Operator_Data *op;
    Expr_Data *lhs;
    Expr_Data *rhs;
};
}
extern MLHashTable<MLGameString, MLGameString, MLHashFun<MLGameString>, 513> localEnv;
extern MLHashTable<MLGameString, MLGameString, MLHashFun<MLGameString>, 513> globalEnv;
extern "C" long __strtol_internal(const char*, char**, int, int);
template<class T> bool ReduceBinOp(IR::Operator_Data*,const T&,const T&,T&);
'''
sources={
 'original-order':context+cbody+'\n'+rbody+'\n'+sbody,
 'reduce-predecessor-control':context+rbody+'\n'+cbody+'\n'+sbody,
}
# Real original addresses, lengths and relocation/literal values in tracked manifests.
configs=[
 ('CanonUnsigned','canonize_try_reduce_unsigned_partial_oct15p.json'),
 ('CanonSigned','canonize_try_reduce_signed_partial_oct15p.json'),
 ('ReduceUnsigned','reduce_binop_unsigned_partial_oct15o.json'),
 ('ReduceSigned','reduce_binop_signed_partial_oct15o.json'),
 ('Concat','concatenate_binop_partial_oct15o.json'),
]
spec=importlib.util.spec_from_file_location('match',R/'scripts/match_probe.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
rows=[]
for name,source in sources.items():
 cpp=O/(name+'.cpp');cpp.write_text(source)
 for snapshot in ('jan25','dec06'):
  for layout,flags in {
   'j4-lab2':['-O2','-frename-registers','-falign-jumps=4','-falign-labels=2','-falign-loops=1'],
   'j8':['-O2','-frename-registers','-falign-jumps=8'],
  }.items():
   obj=O/(name+'-'+snapshot+'-'+layout+'.o')
   p=subprocess.run(['/tmp/vuc-binutils215/g++-'+snapshot+'-as215',*flags,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
   record={'owner':name,'snapshot':snapshot,'layout':layout}
   if p.returncode:record['error']=p.stderr[-200:]
   else:
    for key,cfg in configs:
     try:
      v=m.run(obj,R/'config'/cfg)
      record[key]={'match':v['exact_bytes'],'size':v['length'],'relocations':v['relocations'],'literals':v['verified_literal_bytes']}
     except ValueError as e:record[key]={'error':str(e)}
   rows.append(record)
   print(record,flush=True)
(O/'results.json').write_text(json.dumps(rows,indent=2)+'\n')

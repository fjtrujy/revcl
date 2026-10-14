#!/usr/bin/env python3
"""VUC ASTStmt list 119-byte result home / branch frequency reconstruction.

Authentic AST list loop, original virtual IR::Seq_Data constructor with
16-byte layout and three genuine named relocations. Tests whether temporary
stack ownership of accumulated IR statements restores original register
allocation; C++03 expression-level predictions are diagnostic only.
"""
from pathlib import Path
import itertools,subprocess,importlib.util,collections
ROOT=Path(__file__).resolve().parents[2]
TMP=Path('/tmp/vuc-binutils215/stmt-list-branch-spill-oct14');TMP.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location('match',ROOT/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
base=(ROOT/'vuc/probes/translate_stmt_list.cpp').read_text().replace(
 '#include "../src/ASTNodeList.h"','#include "'+str(ROOT/'vuc/src/ASTNodeList.h')+'"')
sig='IR::Stmt_Data *Translate(ASTNodeList<ASTStmt *> &statements)\n{\n'
at=base.index(sig);prefix=base[:at]
templates={
 'baseline':'''    IR::Stmt_Data *result = 0;
    for (int i=0; i<statements.length; ++i) {
        IR::Stmt_Data *item=Translate(statements.entries[i]);
        if (item) {
            if (result)
                result=(IR::Stmt_Data *)new IR::Seq_Data(result, item);
            else
                result=item;
        }
    }
    return result;''',
 'volatile-result':'''    IR::Stmt_Data *volatile result = 0;
    for (int i=0; i<statements.length; ++i) {
        IR::Stmt_Data *item=Translate(statements.entries[i]);
        if (item) {
            if (result) result=(IR::Stmt_Data *)new IR::Seq_Data(result,item);
            else result=item;
        }
    }
    return result;''',
 'volatile-old':'''    IR::Stmt_Data *volatile result = 0;
    for (int i=0; i<statements.length; ++i) {
        IR::Stmt_Data *item=Translate(statements.entries[i]);
        IR::Stmt_Data *old = result;
        if (item) {
            if (old) result=(IR::Stmt_Data *)new IR::Seq_Data(old,item);
            else result=item;
        }
    }
    return result;''',
 'volatile-skip':'''    IR::Stmt_Data *volatile result = 0;
    for (int i=0; i<statements.length; ++i) {
        IR::Stmt_Data *item=Translate(statements.entries[i]);
        if (!item) continue;
        if (result) result=(IR::Stmt_Data *)new IR::Seq_Data(result,item);
        else result=item;
    }
    return result;''',
 'result-local':'''    IR::Stmt_Data *result = 0;
    ASTNodeList<ASTStmt *> &list = statements;
    for (int i=0; i<list.length; ++i) {
        IR::Stmt_Data *item=Translate(list.entries[i]);
        if (item) {
            if (result) result=(IR::Stmt_Data *)new IR::Seq_Data(result,item);
            else result=item;
        }
    }
    return result;''',
 'result-ref':'''    IR::Stmt_Data *result = 0;
    IR::Stmt_Data *&accum = result;
    for (int i=0; i<statements.length; ++i) {
        IR::Stmt_Data *item=Translate(statements.entries[i]);
        if (item) {
            if (accum) accum=(IR::Stmt_Data *)new IR::Seq_Data(accum,item);
            else accum=item;
        }
    }
    return result;''',
}
flagsets={
 'rj1':['-O2','-frename-registers','-falign-jumps=1'],
 'rj2':['-O2','-frename-registers','-falign-jumps=2'],
 'rj4':['-O2','-frename-registers','-falign-jumps=4'],
 'rj8':['-O2','-frename-registers','-falign-jumps=8'],
 'p':['-O2'],
 'r':['-O2','-frename-registers'],
 'j1-noreorder':['-O2','-frename-registers','-falign-jumps=1','-fno-reorder-blocks']
}
def hint(expr,p):
 return expr if p=='p' else f'__builtin_expect({expr}, {p})'
scores=[];lengths=collections.Counter();n=0
for key,templ in templates.items():
 for p in itertools.product('p01',repeat=3):
  b=templ
  b=b.replace('i<statements.length',hint('i<statements.length',p[0]))
  b=b.replace('i<list.length',hint('i<list.length',p[0]))
  b=b.replace('if (item)',f'if ({hint("item != 0",p[1])})')
  b=b.replace('if (!item)',f'if ({hint("item == 0",p[1])})')
  b=b.replace('if (result)',f'if ({hint("result != 0",p[2])})')
  b=b.replace('if (old)',f'if ({hint("old != 0",p[2])})')
  b=b.replace('if (accum)',f'if ({hint("accum != 0",p[2])})')
  name=key+'-'+''.join(p)
  cpp=TMP/(name+'.cpp');cpp.write_text(prefix+sig+b+'\n}\n')
  for fl,opts in flagsets.items():
   obj=TMP/(name+'-'+fl+'.o')
   proc=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
   n+=1
   if proc.returncode:
    print('FAIL',name,fl,proc.stderr[-90:],flush=True);continue
   line=next((x for x in subprocess.check_output(['nm','-S',str(obj)],text=True).splitlines()
     if x.split()[-1:]==['_Z9TranslateR11ASTNodeListIP7ASTStmtE']),'')
   length=int(line.split()[1],16) if line else -1
   lengths[length]+=1
   if length!=119:continue
   try:
    res=m.run(obj,ROOT/'vuc/config/translate_stmt_list_probe.json')
    scores.append((res['exact_bytes'],length,name,fl,res['different_offsets'][:15],res['relocations']))
    if res['exact']:
     print('NEW_EXACT',cpp,fl,obj,flush=True);raise SystemExit(0)
   except ValueError as e:print('REJECT',name,fl,str(e)[:115],flush=True)
  print('DONE',name,'tested',n,'max',max((a[0] for a in scores),default=0),flush=True)
print('TESTED',n,'LENGTHS',lengths.most_common(16))
print('TOP')
for s in sorted(scores,reverse=True)[:40]:print(s,flush=True)

#!/usr/bin/env python3
"""VUC AST statement list: restore original early-exit / fallthrough CFG.

Source-level prologue guards, do/while and true loop/exit label structure.
Grounded original 119-byte function and 3 actual ELF relocation sites.
"""
from pathlib import Path
import itertools,subprocess,importlib.util,collections
R=Path(__file__).resolve().parents[2];T=Path('/tmp/vuc-binutils215/stmt-preheader-cfg-oct14')
T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
src=(R/'vuc/probes/translate_stmt_list.cpp').read_text().replace(
 '#include "../src/ASTNodeList.h"','#include "'+str(R/'vuc/src/ASTNodeList.h')+'"')
start=src.index('IR::Stmt_Data *Translate(ASTNodeList<ASTStmt *> &statements)')
prefix=src[:start]
inner='''        IR::Stmt_Data *item=Translate(statements.entries[i]);
        if (item) {
            if (result)
                result=(IR::Stmt_Data *)new IR::Seq_Data(result, item);
            else result=item;
        }
        ++i;'''
def cond(expr,p):
 return expr if p=='p' else f'__builtin_expect({expr}, {p})'
variants={}
for guard,li,early in itertools.product('p01','p01','p01'):
 first='i >= statements.length'
 looping='i < statements.length'
 pre = cond(first,guard)
 lo = cond(looping,li)
 srcfirst='''    IR::Stmt_Data *result=0;
    int i=0;
'''
 styles={
 'do-while':srcfirst+f'    if ({pre}) return result;\n    do {{\n{inner}\n    }} while ({lo});\n    return result;\n',
 'while-with-guard':srcfirst+f'    if ({pre}) return result;\n    while ({lo}) {{\n{inner}\n    }}\n    return result;\n',
 'for-with-guard':srcfirst+f'    if ({pre}) return result;\n    for (; {lo}; ) {{\n{inner}\n    }}\n    return result;\n',
 'goto-loop':srcfirst+f'    if ({pre}) goto done;\n  loop_body:\n{inner}\n    if ({lo}) goto loop_body;\n  done:\n    return result;\n',
 'goto-return':srcfirst+f'    if ({cond(looping,guard)}) goto loop_body;\n    return result;\n  loop_body:\n{inner}\n    if ({lo}) goto loop_body;\n    return result;\n',
 'do-false':srcfirst+f'    if (!({cond(looping,guard)})) return result;\n    do {{\n{inner}\n    }} while ({lo});\n    return result;\n',
 'do-return-void':srcfirst+f'    if ({pre}) return result;\n    do {{\n{inner}\n    }} while ({lo});\n    return result;\n',
 }
 for name,body in styles.items():
  k=f'{name}-{guard}{li}{early}'
  item=cond('item != 0',early)
  body=body.replace('if (item)',f'if ({item})')
  variants[k]=body
flags={
'plain':['-O2'],
'ren':['-O2','-frename-registers'],
'plain-j1':['-O2','-falign-jumps=1'],
'plain-j4':['-O2','-falign-jumps=4'],
'ren-j1':['-O2','-frename-registers','-falign-jumps=1'],
'ren-j4':['-O2','-frename-registers','-falign-jumps=4'],
'plain-noreorder':['-O2','-fno-reorder-blocks'],
}
ranks=[];sizes=collections.Counter();n=0
for key,body in variants.items():
 cpp=T/(key+'.cpp');cpp.write_text(prefix+'IR::Stmt_Data *Translate(ASTNodeList<ASTStmt *> &statements)\n{\n'+body+'}\n')
 for flag,opts in flags.items():
  o=T/(key+'-'+flag+'.o')
  cc=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(cpp),'-o',str(o)],capture_output=True,text=True)
  n+=1
  if cc.returncode:
   print('ERROR',key,flag,cc.stderr[-100:],flush=True);continue
  sym=next((s for s in subprocess.check_output(['nm','-S',str(o)],text=True).splitlines()
    if s.split()[-1:]==['_Z9TranslateR11ASTNodeListIP7ASTStmtE']),'')
  size=int(sym.split()[1],16) if sym else -1;sizes[size]+=1
  if size!=119:continue
  try:
   res=m.run(o,R/'vuc/config/translate_stmt_list_probe.json')
   ranks.append((res['exact_bytes'],key,flag,res['different_offsets'][:12],res['relocations']))
   if res['exact']:
    print('NEW_EXACT',cpp,flag,o,flush=True);raise SystemExit(0)
  except ValueError as e: print('REJECT',key,flag,str(e)[:105],flush=True)
 print('DONE',key,'attempts',n,'best',max((r[0] for r in ranks),default=0),flush=True)
print('TESTED',n,'LENGTHS',sizes.most_common(20),'VERIFIED',len(ranks))
print('TOP')
for rec in sorted(ranks,reverse=True)[:38]:print(rec,flush=True)

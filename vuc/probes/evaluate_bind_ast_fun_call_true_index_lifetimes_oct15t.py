#!/usr/bin/env python3
"""VUC Bind(ASTFunCall*) historical C++03 last-index lifetime controls.

Only diagnostic instruction comparisons: the standalone object has 16
unauthenticated compiler literal-section alignment bytes, so strict
promotion remains forbidden despite original-offset comparisons.
"""
from pathlib import Path
import importlib.util, subprocess, json
R=Path(__file__).resolve().parents[1]
O=Path('/tmp/vuc-binutils215/bind-ast-index-lifetime-oct15t')
O.mkdir(exist_ok=True)
src=(R/'probes/BindASTFunCallFreshOct15r.cpp').read_text()
for name in ('MLGameString.h','ASTNodeList.h'):
 src=src.replace('#include "../src/'+name+'"','#include "'+str(R/'src'/name)+'"')
needle='matchStack.entries[matchStack.length-1]->line'
assert src.count(needle)==1
positions={'before-output':'    std::cout << currFileName;',
 'before-error':'    error=true;',
 'before-functionname':'    const char *functionName=',
 'before-compare':'    if (call->callee->entry != symbols.Find',
 'before-bind':'    Bind(call->callee,symbols);'}
patterns={
 'index-calculated':('int index=matchStack.length-1;','matchStack.entries[index]->line'),
 'index-decrement':('int index=matchStack.length; --index;','matchStack.entries[index]->line'),
 'index-predecrement':('int index=matchStack.length;','matchStack.entries[--index]->line'),
 'pointer-calculated':('ASTNode *node=matchStack.entries[matchStack.length-1];','node->line'),
 'pointer-decrement':('ASTNode **last=matchStack.entries+matchStack.length; --last;','(*last)->line'),
 'array-alias':('ASTNode **array=matchStack.entries;','array[matchStack.length-1]->line'),
 'length-alias':('int len=matchStack.length;','matchStack.entries[len-1]->line'),
 'index-unsigned':('unsigned idx=(unsigned)matchStack.length-1;','matchStack.entries[idx]->line')}
forms={'baseline':src}
for pos,anchor in positions.items():
 for kind,(declaration,access) in patterns.items():
  candidate=src.replace(needle,access)
  candidate=candidate.replace(anchor,'    '+declaration+'\n'+anchor,1)
  forms[kind+'-'+pos]=candidate
sp=importlib.util.spec_from_file_location('matcher',R/'scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=json.loads((R/'config/bind_ast_fun_call_partial_oct15r.json').read_text())
raw,elf=m.decomp.load()
cs=next(s for s in elf['sections'] if s['name']=='.text')
rs=next(s for s in elf['sections'] if s['name']=='.rodata')
addr=int(cfg['address'],16)
expected=raw[cs['offset']+addr-cs['address']:cs['offset']+addr-cs['address']+cfg['size']]
rows=[]
for label,source in forms.items():
 cpp=O/(label+'.cpp');cpp.write_text(source)
 obj=O/(label+'.o')
 p=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215','-O2','-frename-registers','-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 row={'source':label}
 if p.returncode:row['error']=p.stderr[-180:]
 else:
  binary=obj.read_bytes();sections,_=m.elf_sections(binary)
  lit=m.section_bytes(binary,sections['.rodata'][1])
  valid=True
  for span in cfg['relocation_literals']['.rodata']:
   off=span['offset'];n=span['size'];a=int(span['address'],16)
   actual=raw[rs['offset']+a-rs['address']:rs['offset']+a-rs['address']+n]
   valid=valid and lit[off:off+n]==actual
  row['referenced_literals_authentic']=valid
  if valid:
   try:
    normalized,count=m.prepare_named_function(binary,cfg['function'],cfg['relocations'],addr,{},cfg['relocation_literals'])
    row['length']=len(normalized);row['relocations']=count
    if len(normalized)==cfg['size']:
     v=m.compare(normalized,expected)
     row['diagnostic_match']=v['exact_bytes'];row['diffs']=v['different_offsets'][:40]
   except ValueError as ex:row['error']=str(ex)
 rows.append(row)
 print({k:v for k,v in row.items() if k!='diffs'},flush=True)
(O/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
print('BEST',sorted([r for r in rows if 'diagnostic_match' in r],key=lambda x:x['diagnostic_match'],reverse=True)[:8],flush=True)

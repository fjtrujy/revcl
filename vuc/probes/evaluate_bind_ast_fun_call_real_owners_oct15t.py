#!/usr/bin/env python3
"""Test authentic accepted Bind list translation-unit context before ASTFunCall.

Recovered original BindLists.cpp has four accepted ASTNodeList loops.
The 304-byte ASTFunCall diagnostic has four unresolved instruction bytes
and 16 unowned compiler-inserted .rodata padding bytes. Source-unit
tests do not fabricate literal ownership; separately report code-only
diagnostics and verify all referenced real original strings.
"""
from pathlib import Path
import json,subprocess,importlib.util
R=Path(__file__).resolve().parents[1]
OUT=Path('/tmp/vuc-binutils215/bind-ast-owner-oct15t');OUT.mkdir(exist_ok=True)
probe=(R/'probes/BindASTFunCallFreshOct15r.cpp').read_text()
probe=probe.replace('#include "../src/MLGameString.h"','#include "'+str(R/'src/MLGameString.h')+'"').replace('#include "../src/ASTNodeList.h"','#include "'+str(R/'src/ASTNodeList.h')+'"')
body=probe[probe.index('struct ASTNode {'):]
real='#include <iostream>\n#include "'+str(R/'src/BindLists.cpp')+'"\n'
variants={
 'isolated':probe,
 'accepted-bind-lists-predecessor':real+body,
 'accepted-bind-lists-after':probe+'\n#include "'+str(R/'src/BindLists.cpp')+'"\n',
}
sp=importlib.util.spec_from_file_location('m',R/'scripts/match_probe.py');m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=json.loads((R/'config/bind_ast_fun_call_partial_oct15r.json').read_text())
original,target=m.decomp.load();sec=next(s for s in target['sections'] if s['name']=='.text');addr=int(cfg['address'],16)
expected=original[sec['offset']+addr-sec['address']:sec['offset']+addr-sec['address']+cfg['size']]
ro=next(s for s in target['sections'] if s['name']=='.rodata')
rows=[]
for name,source in variants.items():
 cpp=OUT/(name+'.cpp');cpp.write_text(source)
 for compiler in ('jan25','dec06'):
  obj=OUT/(name+'-'+compiler+'.o')
  p=subprocess.run(['/tmp/vuc-binutils215/g++-'+compiler+'-as215','-O2','-frename-registers','-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
  row={'variant':name,'compiler':compiler}
  if p.returncode:row['error']=p.stderr[-190:]
  else:
   data=obj.read_bytes()
   sections,_=m.elf_sections(data)
   literals=m.section_bytes(data,sections['.rodata'][1])
   for literal in cfg['relocation_literals']['.rodata']:
    off,n=literal['offset'],literal['size'];a=int(literal['address'],16)
    test=original[ro['offset']+a-ro['address']:ro['offset']+a-ro['address']+n]
    if literals[off:off+n]!=test:
     row['error']='referenced original literal differs at '+hex(off);break
   if 'error' not in row:
    try:
     norm,rel=m.prepare_named_function(data,cfg['function'],cfg['relocations'],addr,{},cfg['relocation_literals'])
     row['length']=len(norm);row['relocations']=rel
     row['diagnostic_code']=m.compare(norm,expected)['exact_bytes'] if len(norm)==cfg['size'] else 0
     row['unowned_literal_padding']=len(literals)-sum(x['size'] for x in cfg['relocation_literals']['.rodata'])
    except ValueError as ex:row['error']=str(ex)
  rows.append(row);print(row,flush=True)
(OUT/'results.json').write_text(json.dumps(rows,indent=2)+'\n')

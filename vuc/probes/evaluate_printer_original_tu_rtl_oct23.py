#!/usr/bin/env python3
"""Track the callee register across the authentic recoverable Printer source unit.

Use real Print(ASTFunCall), Printer_UglyPrint, compiler-generated iostream
startup and existing exact AST-list template definitions/instantiations.
Missing large AST printers are NOT replaced with artificial filler.
"""
from pathlib import Path
import importlib.util,subprocess,json,re
ROOT=Path(__file__).resolve().parents[1]
WORK=Path('/tmp/vuc-printer-original-owner-rtl-20261023');WORK.mkdir(exist_ok=True)
base=(ROOT/'src/Printer.cpp').read_text().replace(
 '#include "../src/ASTNodeList.h"','#include "'+str(ROOT/'src/ASTNodeList.h')+'"')
ast=(ROOT/'src/PrintASTLists.cpp').read_text()
template=ast[ast.index('template <class T>\nvoid Print('):]
header,body=base.split('void Print(ASTFunCall *call, std::ostream &output)',1)
body='void Print(ASTFunCall *call, std::ostream &output)'+body
prototypes=('void Print(ASTExpr *, std::ostream &);\n'
            'void Print(ASTStmt *, std::ostream &);\n'
            'void Print(ASTDecl *, std::ostream &);\n')
# Definitions preserve all original-member function implementations.
contexts={'owner':base,
 'owner-list-after':header+prototypes+body+'\n'+template,
 'list-before-owner':header+prototypes+template+'\n'+body}
flags={'plain':['-O2'],
       'startup-exact':['-O2','-frename-registers','-fuse-cxa-atexit','-ffunction-sections'],
       'startup-plain':['-O2','-fuse-cxa-atexit','-ffunction-sections']}
sp=importlib.util.spec_from_file_location('vuc_match_real_owner',ROOT/'scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
rows=[]
for context,source in contexts.items():
 cpp=WORK/(context+'.cpp');cpp.write_text(source)
 for compiler in ('jan25','dec06'):
  for flag,arguments in flags.items():
   key=context+'-'+compiler+'-'+flag;out=WORK/key;out.mkdir(exist_ok=True)
   # -da writes all GCC RTL stages into an isolated directory.
   obj=out/'unit.o'
   cp=subprocess.run(['/tmp/vuc-binutils215/g++-'+compiler+'-as215',*arguments,
     '-da','-c',str(cpp),'-o',str(obj)],cwd=out,capture_output=True,text=True)
   row={'case':key}
   if cp.returncode:row['error']=cp.stderr[-320:]
   else:
    for label,config in (('call','print_ast_fun_context_oct14_probe.json'),
       ('ugly','ugly_printer_unit_oct13_probe.json'),
       ('startup','iostream_global_printer_oct13_probe.json'),
       ('expr-list','print_list_expr_exact_probe.json')):
     try:row[label]=m.run(obj,ROOT/'config'/config)
     except Exception as error:row[label]={'error':str(error)}
    rtl=next(out.glob('*.24.greg'),None)
    if rtl:
     text=rtl.read_text(errors='replace')
     f=re.search(r';; Function void Print\(ASTFunCall\*.*?(?=;; Function|\Z)',text,re.S)
     if f:
      insn=re.search(r'\(set \(reg:SI (\d+) (\w+)(?: \[\d+\])?\)\s*\(mem/s:SI.*?<variable>\.callee',f.group(),re.S)
      row['greg_callee']=insn.group(1)+' '+insn.group(2) if insn else 'unparsed'
   rows.append(row)
   print(key,'call',row.get('call',{}).get('exact_bytes'),
     'ugly',row.get('ugly',{}).get('exact_bytes'),
     'startup',row.get('startup',{}).get('exact_bytes'),
     'list',row.get('expr-list',{}).get('exact_bytes'),
     'greg',row.get('greg_callee'),flush=True)
(WORK/'results.json').write_text(json.dumps(rows,indent=2)+'\n')

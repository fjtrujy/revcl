#!/usr/bin/env python3
"""Test ABI-plausible Print return contracts; Itanium name omits return types.

Original VUC ASTFunCall printer ends in an ostream insertion tail call.
A reference return type is therefore a historically plausible hypothesis.
"""
from pathlib import Path
import subprocess,importlib.util
R=Path(__file__).resolve().parents[1];O=Path('/tmp/vuc-print-funcall-return-oct15v');O.mkdir(exist_ok=True)
base=(R/'src/Printer.cpp').read_text().replace('#include "../src/ASTNodeList.h"','#include "'+str(R/'src/ASTNodeList.h')+'"')
start='void Print(ASTFunCall *call, std::ostream &output)';body='    output << ")";\n}'
forms={
 'void-baseline':('void',body),
 'return-reference-expression':('std::ostream &','    return output << ")";\n}'),
 'return-reference-statement':('std::ostream &','    output << ")";\n    return output;\n}'),
 'return-const-ref-expression':('const std::ostream &','    return output << ")";\n}'),
 'return-pointer-expression':('std::ostream *','    return &(output << ")");\n}'),
 'return-pointer-statement':('std::ostream *','    output << ")";\n    return &output;\n}'),
 'return-void-expression':('void','    return (void)(output << ")");\n}'),
 'return-reference-alias':('std::ostream &','    std::ostream &stream=(output << ")");\n    return stream;\n}'),
 'return-bool-stream':('bool','    return (bool)(output << ")");\n}'),
 'return-int-zero':('int','    output << ")";\n    return 0;\n}'),
}
sp=importlib.util.spec_from_file_location('match',R/'scripts/match_probe.py');m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
for name,(typ,end) in forms.items():
 path=O/(name+'.cpp');src=base.replace(start,typ+' Print(ASTFunCall *call, std::ostream &output)',1).replace(body,end,1);path.write_text(src)
 for cc in ('jan25',):
  for fs,fl in [('O2',['-O2']),('rename',['-O2','-frename-registers'])]:
   obj=O/(name+'-'+cc+'-'+fs+'.o');p=subprocess.run(['/tmp/vuc-binutils215/g++-'+cc+'-as215',*fl,'-c',str(path),'-o',str(obj)],capture_output=True,text=True)
   if p.returncode:print('COMPILE_FAIL',name,cc,fs,p.stderr[-200:],flush=True);continue
   try:
    score=m.run(obj,R/'config/print_ast_fun_context_oct14_probe.json');print('RESULT',name,cc,fs,score['exact_bytes'],score['length'],score['different_offsets'][:18],flush=True)
    if score['exact']:print('BYTE_EXACT',name,cc,fs,flush=True);(O/'EXACT.txt').write_text(name+' '+cc+' '+fs+'\n')
   except Exception as e:print('REJECT',name,cc,fs,str(e)[:180],flush=True)

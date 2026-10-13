#!/usr/bin/env python3
"""Reconstruct the original GCC 3.3.3 constant tab-width scanner source form."""
from pathlib import Path
import subprocess,importlib.util
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/tab-constants-forms-oct13');T.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
source=(R/'vuc/probes/asm_output_tab_constants.cpp').read_text()
header=source[:source.index('void AsmOutput_TabConstants(')]
body=source[source.index('void AsmOutput_TabConstants('):]
forms={
 'original':body,
 'signed-count':body.replace('int count = list.instructions_.size();','int count = static_cast<int>(list.instructions_.size());'),
 'unsigned-count':body.replace('int count = list.instructions_.size();','unsigned count = list.instructions_.size();').replace('for (int i','for (unsigned i'),
 'size_t-count':body.replace('int count = list.instructions_.size();','std::size_t count = list.instructions_.size();').replace('for (int i','for (std::size_t i'),
 'vector-reference':body.replace('    int count =', '    std::vector<Instruction *>& instructions = list.instructions_;\n    int count =').replace('list.instructions_','instructions',4).replace('instructions = instructions','instructions = list.instructions_'),
 'list-pointer':body.replace('    int count =', '    InstructionList *table = &list;\n    int count =').replace('list.instructions_','table->instructions_'),
 'vector-pointer':body.replace('    int count =', '    std::vector<Instruction *> *vec = &list.instructions_;\n    int count =').replace('list.instructions_','(*vec)').replace('&(*vec)','&list.instructions_'),
 'index-temporary':body.replace('        const char *text = list.instructions_[i]->ToString();','        Instruction *current = list.instructions_[i];\n        const char *text = current->ToString();'),
 'begin-index':body.replace('list.instructions_[i]->ToString()','(*(list.instructions_.begin() + i))->ToString()'),
 'begin-subs':body.replace('list.instructions_[i]->ToString()','list.instructions_.begin()[i]->ToString()'),
 'bound-reversed':body.replace('i < count','count > i'),
 'preincrement':body.replace('++i','i++'),
 'width-local':body.replace('int width = delimiter - text;', 'int width = static_cast<int>(delimiter - text);'),
 'width-unsigned':body.replace('int width = delimiter - text;', 'unsigned width = delimiter - text;'),
 'width-named':body.replace('int width = delimiter - text;', 'const char *begin = text;\n            int width = delimiter - begin;'),
 'guard-noelse':body.replace('        if (delimiter) {\n            int width', '        if (!delimiter) continue;\n        {\n            int width'),
 'guard-early':body.replace('        if (delimiter) {\n            int width = delimiter - text;\n            if (width > constantsTabStop)\n                constantsTabStop = width;\n        }','        if (!delimiter) continue;\n        int width = delimiter - text;\n        if (width > constantsTabStop) constantsTabStop = width;'),
 'strstr-if':body.replace('        if (!delimiter)\n            delimiter = std::strstr(text, ".equ");','        if (delimiter == 0) {\n            delimiter = std::strstr(text, ".equ");\n        }'),
 'strstr-conditional':body.replace('        if (!delimiter)\n            delimiter = std::strstr(text, ".equ");','        delimiter = delimiter ? delimiter : std::strstr(text, ".equ");'),
 'const-vector':body.replace('    int count =','    const std::vector<Instruction *> &vec = list.instructions_;\n    int count =').replace('list.instructions_.size()','vec.size()').replace('list.instructions_[i]','vec[i]'),
 'const-bound':body.replace('int count =','const int count ='),
 'explicit-count':body.replace('int count = list.instructions_.size();','int count = (list.instructions_.end() - list.instructions_.begin());'),
}
flags={'plain':['-O2','-falign-jumps=4'],'ren':['-O2','-frename-registers','-falign-jumps=4'],'j8':['-O2','-falign-jumps=8'],'j16':['-O2','-falign-jumps=16']}
rank=[]
for name,form in forms.items():
 src=T/(name+'.cpp');src.write_text(header+form)
 for opt,fl in flags.items():
  obj=T/(name+'-'+opt+'.o')
  cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*fl,'-c',str(src),'-o',str(obj)],capture_output=True,text=True)
  if cp.returncode:print('COMPILE_FAIL',name,opt,cp.stderr[-130:],flush=True);continue
  try:
   result=m.run(obj,R/'vuc/config/asm_output_tab_constants_probe.json')
   rank.append((result['exact_bytes'],name,opt,result['length']))
   if result['exact']:print('BYTE EXACT',name,opt,src,flush=True);raise SystemExit(0)
  except ValueError as e:rank.append((0,name,opt,str(e)[:90]))
 print('checked',name,flush=True)
for record in sorted(rank,reverse=True)[:32]:print('BEST',record)

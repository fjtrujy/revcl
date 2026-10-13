#!/usr/bin/env python3
"""Search original-sized STL SymbolScope iterator copying with GCC 3.3.3.

Only genuine std::__uninitialized_copy_aux instantiation and normal
compiler alignments; all six real EH/constructor relocations must resolve.
"""
import itertools,subprocess,importlib.util
from pathlib import Path
R=Path(__file__).resolve().parents[2];T=Path("/tmp/vuc-binutils215/scope-iterator-alignment-next");T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
source=R/'vuc/probes/scope_iterator_copy_oct12.cpp'
cfg=R/'vuc/config/scope_iterator_copy_oct12_probe.json'
rows=[]
for jump,label,loop in itertools.product((1,2,4),(1,2,3,4,8),(1,2,4,8)):
 key=f"j{jump}-l{label}-loop{loop}";opt=['-O2','-frename-registers',f'-falign-jumps={jump}',f'-falign-labels={label}',f'-falign-loops={loop}']
 obj=T/(key+'.o')
 cc=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opt,'-c',str(source),'-o',str(obj)],capture_output=True,text=True)
 if cc.returncode:print('ERROR',key,cc.stderr[-95:],flush=True);continue
 symbol=next((r for r in subprocess.check_output(['nm','-S',str(obj)],text=True).splitlines() if r.endswith('_ZSt24__uninitialized_copy_auxIN9__gnu_cxx17__normal_iteratorIP11MLHashTableI12MLGameStringP16SymbolTableEntry9MLHashFunIS3_ELi513EESt6vectorIS8_SaIS8_EEEESD_ET0_T_SF_SE_12__false_type')),"")
 size=int(symbol.split()[1],16) if symbol else -1
 if size==149:
  try:
   score=m.run(obj,cfg)
   rows.append((score['exact_bytes'],size,key))
   print('ORIGINAL_SIZE',key,score['exact_bytes'],score['different_offsets'][:14],flush=True)
   if score['exact']:print('NEW_EXACT',key);break
  except ValueError as e:print('REJECT',key,str(e)[:105],flush=True)
 else:rows.append((0,size,key))
print('BEST')
for row in sorted(rows,reverse=True)[:25]:print(row)

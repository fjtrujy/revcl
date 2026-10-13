#!/usr/bin/env python3
"""Test real original AlignConstant neighbor in AsmOutput historical source unit.

Both original functions and 127 source-unit string literal bytes are independently verified.
"""
from pathlib import Path
import importlib.util,json,subprocess
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/asm-align-constant-context-oct13')
T.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
base=(R/'vuc/src/AsmOutputUnit.cpp').read_text()
other=(R/'vuc/src/AlignConstant.cpp').read_text()
other=other[other.index('#include <cstring>'):]
needle='\n#include <cstring>\n#include <vector>\n'
assert needle in base
old='''            int padding = alignment - offset;
            for (int i = 0; i < padding; ++i)
                space[i] = ' ';
            space[padding] = 0;
            strcat(space, text + offset);'''
new='''            int i;
            for (i = 0; i < alignment - offset; ++i) space[i] = ' ';
            space[i] = 0;
            strcat(space, text + (space - formatted));'''
assert old in base
configs={}
for key,fn in [('align','align_string_probe.json'),('tab','asm_output_tab_constants_probe.json'),('align-constant','align_constant_exact_probe.json')]:
 cfg=json.loads((R/'vuc/config'/fn).read_text())
 cfg['relocations'].pop('.rodata',None)
 cfg['relocation_literals']={'.rodata':[
  {'offset':0,'size':92,'address':'0x080720a0'},
  {'offset':92,'size':19,'address':'0x080721c4'},
  {'offset':111,'size':3,'address':'0x080721d7'},
  {'offset':114,'size':8,'address':'0x080721f0'},
  {'offset':122,'size':5,'address':'0x080721fb'}]}
 configs[key]=cfg
flags={'rename':['-O2','-frename-registers','-falign-jumps=4'], 'plain':['-O2','-falign-jumps=4']}
for shape,raw in [('base',base),('correct-loop',base.replace(old,new))]:
 for location in ('before-tab','after-tab'):
  unit=raw.replace(needle,'\n'+other+'\n'+needle,1) if location=='before-tab' else raw+'\n'+other+'\n'
  stem=shape+'-'+location
  src=T/(stem+'.cpp');src.write_text(unit)
  for flname,opts in flags.items():
   obj=T/(stem+'-'+flname+'.o')
   cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(src),'-o',str(obj)],capture_output=True,text=True)
   if cp.returncode:
    print('COMPILE_ERROR',stem,flname,cp.stderr[-230:],flush=True)
    continue
   for key in ('align','tab','align-constant'):
    cfg=T/(stem+'-'+key+'.json');cfg.write_text(json.dumps(configs[key]))
    try:
     score=m.run(obj,cfg)
     print(stem,flname,key,f'{score["exact_bytes"]}/{score["length"]}',score['different_offsets'][:15],score['verified_literal_bytes'],flush=True)
     if key=='align' and score['exact']:print('ALIGN EXACT',stem,flname,src,flush=True)
    except ValueError as e: print('REJECT',stem,flname,key,str(e)[:200],flush=True)

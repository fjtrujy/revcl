#!/usr/bin/env python3
"""Verify real AsmOutput prologue and epilogue beside byte-exact TabConstants."""
from pathlib import Path
import json,subprocess,importlib.util
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/asm-unit-neighbor-oct13');T.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
source=R/'vuc/src/AsmOutputUnit.cpp'
config_literals=[{'offset':0,'size':92,'address':'0x080720a0'},{'offset':92,'size':19,'address':'0x080721c4'},{'offset':111,'size':8,'address':'0x080721f0'},{'offset':119,'size':5,'address':'0x080721fb'}]
for flag,opts in {'rename':['-O2','-frename-registers','-falign-jumps=4'],'plain':['-O2','-falign-jumps=4'],'ren-func':['-O2','-frename-registers','-falign-jumps=4','-ffunction-sections']}.items():
 obj=T/(flag+'.o');cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(source),'-o',str(obj)],capture_output=True,text=True)
 if cp.returncode:print(flag,cp.stderr[-150:],flush=True);continue
 for label,config in [('prologue','asm_output_unit_oct13_probe.json'),('epilogue','asm_output_epilogue_probe.json'),('tab','asm_output_tab_constants_probe.json')]:
  data=json.loads((R/'vuc/config'/config).read_text())
  data['source']='vuc/src/AsmOutputUnit.cpp'
  data['flags']=opts
  data['relocations'].pop('.rodata',None)
  data['relocation_literals']={'.rodata':config_literals}
  cfg=T/(flag+'-'+label+'.json');cfg.write_text(json.dumps(data,indent=2))
  try:
   result=m.run(obj,cfg)
   print(flag,label,f"{result['exact_bytes']}/{result['length']}",result['different_offsets'],result['relocations'],flush=True)
  except ValueError as e:print(flag,label,'REJECT',str(e)[:130],flush=True)

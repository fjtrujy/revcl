#!/usr/bin/env python3
"""Bounded historical GCC alignment matrix for original AddLineComment layout."""
import importlib.util,itertools,subprocess
from pathlib import Path
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/instruction-line-align-matrix-oct13');T.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
source=R/'vuc/probes/instruction_line_with_unit_oct13.cpp'
config=R/'vuc/config/instruction_add_line_comment_context_oct13_probe.json'
records=[]
for jump,label,loop in itertools.product((2,4,8),(1,2,4),(1,2,4)):
 for rename in (True,False):
  flag=['-O2']+(['-frename-registers'] if rename else []) + [f'-falign-jumps={jump}',f'-falign-labels={label}',f'-falign-loops={loop}']
  name=f'j{jump}-l{label}-loop{loop}-'+('ren' if rename else 'plain')
  obj=T/(name+'.o')
  cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*flag,'-c',str(source),'-o',str(obj)],capture_output=True,text=True)
  if cp.returncode:print(name,'COMPILER',cp.stderr[-100:],flush=True);continue
  syms=subprocess.run(['nm','-S',str(obj)],capture_output=True,text=True).stdout
  line=next((x for x in syms.splitlines() if '_ZN15InstructionList14AddLineCommentEi' in x),'')
  size=int(line.split()[1],16) if line else 0
  if size==162:
   try:
    result=m.run(obj,config)
    records.append((result['exact_bytes'],name,size))
    if result['exact']:
     print('EXACT',name,flag,flush=True);raise SystemExit(0)
   except ValueError as e: print('MATCH_REJECT',name,str(e)[:100],flush=True)
  else:records.append((0,name,size))
 print('tested',jump,label,loop,flush=True)
for rec in sorted(records,reverse=True)[:24]:print('BEST',rec)

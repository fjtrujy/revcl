#!/usr/bin/env python3
"""Test authentic AsmOutput globals and neighboring stream routines under GCC 3.3."""
import importlib.util
from pathlib import Path
import subprocess
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/asm-unit-oct13');T.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
base=(R/'vuc/probes/asm_output_real_unit_oct13.cpp').read_text()
with_glob=base.replace('#include <fstream>','#include <fstream>\n#include <iostream>\nint constantsTabStop;')
variants={
 'paired':base,
 'with-global':with_glob,
 'global-after':base+'\n#include <iostream>\nint constantsTabStop;\n',
 'iostream-only':base.replace('#include <fstream>','#include <iostream>\n#include <fstream>'),
 'with-align-constant':base+'\n#include "'+str(R/'vuc/src/AlignConstant.cpp')+'"\n',
 'with-global-and-align':with_glob+'\n#include "'+str(R/'vuc/src/AlignConstant.cpp')+'"\n',
}
flags={
 'rename':['-O2','-frename-registers'],
 'norename':['-O2'],
 'ren-j1':['-O2','-frename-registers','-falign-jumps=1'],
 'ren-j4':['-O2','-frename-registers','-falign-jumps=4'],
 'ren-functions':['-O2','-frename-registers','-ffunction-sections','-fuse-cxa-atexit'],
}
rank=[]
for name,code in variants.items():
 source=T/(name+'.cpp');source.write_text(code)
 for kind,fl in flags.items():
  obj=T/(name+'-'+kind+'.o')
  cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*fl,'-c',str(source),'-o',str(obj)],capture_output=True,text=True)
  if cp.returncode:
   rank.append((0,name,kind,cp.stderr[-110:]));continue
  try:
   r=m.run(obj,R/'vuc/config/asm_output_unit_oct13_probe.json')
   rank.append((r['exact_bytes'],name,kind,r['length']))
   if r['exact']:
    print('EXACT',name,kind,flush=True)
    raise SystemExit(0)
  except ValueError as e:rank.append((0,name,kind,str(e)[:95]))
 print('checked',name,flush=True)
for x in sorted(rank,reverse=True)[:15]:print(*x)

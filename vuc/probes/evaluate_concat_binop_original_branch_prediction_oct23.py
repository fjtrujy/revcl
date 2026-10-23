#!/usr/bin/env python3
"""Transfer confirmed ReduceBinOp probability insight to real ConcatenateBinOp.

Change one authentic C++03 operation guard at a time, preserving original
ELF-referenced operator strings, all source literals, and signed enum values.
Only original 346-byte, relocation/literal-authenticated scores count.
"""
from pathlib import Path
import json,subprocess,importlib.util
R=Path(__file__).resolve().parents[1]
T=Path('/tmp/vuc-concat-opcode-prediction-oct23');T.mkdir(exist_ok=True)
sp=importlib.util.spec_from_file_location('matcher',R/'scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
source=(R/'probes/ConcatenateBinOpNewOct15o.cpp').read_text()
source=source.replace('#include "../src/MLGameString.h"','#include "'+str(R/'src/MLGameString.h')+'"')
rows=[]
for opcode in (13,12,11,10,9,8,7,6,5,4,3,2,1,0):
 for prediction in (0,1):
  needle=f'(op->operation == {opcode})'
  assert source.count(needle)==1
  text=source.replace(needle,f'(__builtin_expect(op->operation == {opcode}, {prediction}))')
  key=f'op{opcode}-prediction{prediction}'
  cpp=T/(key+'.cpp');cpp.write_text(text)
  row={'key':key}
  for compiler in ('jan25','dec06'):
   obj=T/(key+'-'+compiler+'.o')
   options=['-O2','-frename-registers','-falign-jumps=4','-falign-labels=2','-falign-loops=1']
   cp=subprocess.run(['/tmp/vuc-binutils215/g++-'+compiler+'-as215',*options,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
   if cp.returncode:row[compiler]={'error':cp.stderr[-300:]};continue
   try:
    score=m.run(obj,R/'config/concatenate_binop_partial_oct15o.json')
    row[compiler]={'matched':score['exact_bytes'],'size':score['length'],'relocs':score['relocations'],'literals':score['verified_literal_bytes']}
    if score['exact_bytes']>223:print('IMPROVED',key,compiler,score['exact_bytes'],flush=True)
   except ValueError as err:row[compiler]={'error':str(err)}
  rows.append(row)
  print(key,row['jan25'],flush=True)
(T/'results.json').write_text(json.dumps(rows,indent=2)+'\n')

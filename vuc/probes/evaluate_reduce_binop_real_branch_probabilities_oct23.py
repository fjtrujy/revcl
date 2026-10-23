#!/usr/bin/env python3
"""Test genuine C++03 opcode-guard likelihood at original multiplication/division boundary.

Both signed/unsigned templates must preserve all operation semantics, original
315-byte size and relocation-free original-offset matching. No opcode padding
or register-forcing source code.
"""
from pathlib import Path
import importlib.util,json,re,subprocess
ROOT=Path(__file__).resolve().parents[2];R=ROOT/'vuc'
T=Path('/tmp/vuc-reduce-binop-guard-control-oct23');T.mkdir(exist_ok=True)
spec=importlib.util.spec_from_file_location('matcher',R/'scripts/match_probe.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
baseline=(R/'probes/ReduceBinOpNewOct15o.cpp').read_text()
rows=[]
for opcode in (13,12,11,10,9,8,7,6,5,4,3,2,1,0):
 for likely in (0,1):
  needle='(operation=='+str(opcode)+')'
  replacement=f'(__builtin_expect(operation=={opcode}, {likely}))'
  assert needle in baseline
  src=baseline.replace(needle,replacement)
  name=f'op{opcode}-likely{likely}'
  cpp=T/(name+'.cpp');cpp.write_text(src)
  row={'variant':name}
  for snapshot in ('jan25','dec06'):
   obj=T/(name+'-'+snapshot+'.o')
   command=['/tmp/vuc-binutils215/g++-'+snapshot+'-as215','-O2','-frename-registers','-falign-jumps=8','-c',str(cpp),'-o',str(obj)]
   result=subprocess.run(command,capture_output=True,text=True)
   scores={}
   if result.returncode:row[snapshot]={'error':result.stderr[-350:]};continue
   for typ in ('unsigned','signed'):
    try:
     score=m.run(obj,R/'config'/('reduce_binop_'+typ+'_partial_oct15o.json'))
     scores[typ]=score['exact_bytes']
     if score['exact_bytes']>300:print('IMPROVED',name,snapshot,typ,score['exact_bytes'],flush=True)
    except (ValueError,KeyError) as exc:scores[typ]='REJECT '+str(exc)
   row[snapshot]=scores
  rows.append(row)
  print(name,row['jan25'].get('unsigned'),row['jan25'].get('signed'),flush=True)
(T/'results.json').write_text(json.dumps(rows,indent=2)+'\n')

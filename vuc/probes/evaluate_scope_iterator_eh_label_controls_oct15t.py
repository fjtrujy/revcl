#!/usr/bin/env python3
"""Narrow exception-label alignment controls for original SymbolTable TU.

Real STL Scope copy is 145/147/151 with three historical align-jump
values, while target is 149. This bounded matrix tests only the two
compiler alignments directly implicated by the disassembly: entry/catch
labels and cleanup loops. It also requires original 746-byte neighboring
Scope vector insertion, avoiding any arbitrary output-length match.
"""
from pathlib import Path
import subprocess,json
R=Path(__file__).resolve().parents[1]
O=Path('/tmp/vuc-binutils215/scope-iterator-eh-label-controls-oct15t');O.mkdir(exist_ok=True)
source=R/'probes/ScopeNormalIteratorRealOrderOct15t.cpp'
cases=[
 ('j1',('-falign-jumps=1',)),
 ('j1-label1',('-falign-jumps=1','-falign-labels=1')),
 ('j1-label2',('-falign-jumps=1','-falign-labels=2')),
 ('j1-label4',('-falign-jumps=1','-falign-labels=4')),
 ('j1-loop1',('-falign-jumps=1','-falign-loops=1')),
 ('j1-loop2',('-falign-jumps=1','-falign-loops=2')),
 ('j1-loop4',('-falign-jumps=1','-falign-loops=4')),
 ('j4-label1',('-falign-jumps=4','-falign-labels=1')),
 ('j4-label2',('-falign-jumps=4','-falign-labels=2')),
 ('j4-loop1',('-falign-jumps=4','-falign-loops=1')),
 ('j4-loop2',('-falign-jumps=4','-falign-loops=2')),
 ('j8-label1',('-falign-jumps=8','-falign-labels=1')),
 ('j8-label2',('-falign-jumps=8','-falign-labels=2')),
 ('j8-loop1',('-falign-jumps=8','-falign-loops=1')),
 ('j8-loop2',('-falign-jumps=8','-falign-loops=2')),
]
results=[]
for tag,flags in cases:
 for snap in ('jan25','dec06'):
  obj=O/(tag+'-'+snap+'.o')
  cp=subprocess.run(['/tmp/vuc-binutils215/g++-'+snap+'-as215','-O2','-frename-registers',*flags,'-c',str(source),'-o',str(obj)],capture_output=True,text=True)
  row={'case':tag,'snapshot':snap,'flags':flags}
  if cp.returncode:row['error']=cp.stderr[-150:]
  else:
   nm=subprocess.run(['nm','-S','--defined-only',str(obj)],capture_output=True,text=True)
   sy=[x.split() for x in nm.stdout.splitlines() if len(x.split())>=4]
   for name,sub in [('insert','_M_insert_aux'),('copy','__uninitialized_copy_aux')]:
    x=next((r for r in sy if sub in r[-1]),None)
    row[name]=int(x[1],16) if x else -1
  results.append(row);print(row,flush=True)
(O/'results.json').write_text(json.dumps(results,indent=2)+'\n')

#!/usr/bin/env python3
"""Bounded original string vector EH-loop layout in genuine source unit.

Original VUC ELF: std::vector<std::string>::_M_insert_aux 668 bytes
before already byte-exact 161-byte std::__uninitialized_copy_aux.
The authentic compiler source is the real GCC 3.3.3 instantiation;
test only alignment of identified insertion/copy-loop/exception labels.
Do not select unless original 668 bytes and 161/161 predecessor pass.
"""
from pathlib import Path
import importlib.util,subprocess,json
R=Path(__file__).resolve().parents[1];O=Path('/tmp/vuc-binutils215/string-vector-original-owner-controls-oct15t');O.mkdir(exist_ok=True)
src=R/'probes/StringVectorInsertAuxRealOrderOct15s.cpp'
sp=importlib.util.spec_from_file_location('match',R/'scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
copycfg=R/'config/stl_string_uninitialized_probe.json'
cases={
 'default':(),
 'j4':('-falign-jumps=4',),
 'j4-loop1':('-falign-jumps=4','-falign-loops=1'),
 'j4-loop2':('-falign-jumps=4','-falign-loops=2'),
 'j4-loop4':('-falign-jumps=4','-falign-loops=4'),
 'j4-label1':('-falign-jumps=4','-falign-labels=1'),
 'j4-label2':('-falign-jumps=4','-falign-labels=2'),
 'j4-label2-loop1':('-falign-jumps=4','-falign-labels=2','-falign-loops=1'),
 'j4-label2-loop2':('-falign-jumps=4','-falign-labels=2','-falign-loops=2'),
 'j4-label2-loop4':('-falign-jumps=4','-falign-labels=2','-falign-loops=4'),
 'j8':('-falign-jumps=8',),
 'j8-loop1':('-falign-jumps=8','-falign-loops=1'),
 'j8-loop2':('-falign-jumps=8','-falign-loops=2'),
 'j8-loop4':('-falign-jumps=8','-falign-loops=4'),
 'j8-label2':('-falign-jumps=8','-falign-labels=2'),
}
rows=[]
for tag,opts in cases.items():
 for snap in ('jan25','dec06'):
  obj=O/(tag+'-'+snap+'.o')
  proc=subprocess.run(['/tmp/vuc-binutils215/g++-'+snap+'-as215','-O2','-frename-registers',*opts,'-c',str(src),'-o',str(obj)],capture_output=True,text=True)
  row={'case':tag,'compiler':snap,'flags':opts}
  if proc.returncode:row['error']=proc.stderr[-130:]
  else:
   nm=subprocess.run(['nm','-S','--defined-only',str(obj)],capture_output=True,text=True)
   sy=[line.split() for line in nm.stdout.splitlines() if len(line.split())>=4]
   insert=next((x for x in sy if '_M_insert_aux' in x[-1]),None)
   row['insert_length']=int(insert[1],16) if insert else -1
   try:
    v=m.run(obj,copycfg)
    row['copy']=(v['length'],v['exact_bytes'],v['relocations'])
   except ValueError as ex:row['copy']=str(ex)
  rows.append(row);print(row,flush=True)
(O/'results.json').write_text(json.dumps(rows,indent=2)+'\n')

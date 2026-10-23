#!/usr/bin/env python3
"""Diagnose vector success-store pass ordering under GCC3.3 scheduler controls.

All candidates compile genuine BeginScope and the authentic historical
vector.tcc: no header rewriting or machine-code patching. This is compiler
diagnostic evidence, not an authorized production optimization change.
"""
from pathlib import Path
import subprocess,importlib.util,json,re
R=Path(__file__).resolve().parents[1]
T=Path('/tmp/vuc-vector-scheduler-controls-20261023');T.mkdir(exist_ok=True)
spec=importlib.util.spec_from_file_location('matcher',R/'scripts/match_probe.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
modes={
 'default':[],
 'disable-sched1':['-fno-schedule-insns'],
 'disable-sched2':['-fno-schedule-insns2'],
 'disable-spec':['-fno-sched-spec'],
 'disable-interblock':['-fno-sched-interblock'],
 'disable-spec-load':['-fno-sched-spec-load'],
 'enable-spec-load':['-fsched-spec-load'],
 'enable-dangerous':['-fsched-spec-load-dangerous'],
 'disable-pressure':['-fno-sched-spec-load','-fno-sched-interblock'],
 'disable-gcse':['-fno-gcse'],
 'disable-rename':['-fno-rename-registers'],
}
rows=[]
for snap in ('jan25','dec06'):
 for label,extra in modes.items():
  tag=snap+'-'+label
  obj=T/(tag+'.o')
  cmd=['/tmp/vuc-binutils215/g++-'+snap+'-as215',
       '-O2','-frename-registers','-falign-jumps=1',*extra,'-c',
       str(R/'probes/ScopeVectorInsertionOriginalUnitOct15i.cpp'),'-o',str(obj)]
  cp=subprocess.run(cmd,capture_output=True,text=True)
  result={'case':tag,'flags':cmd[1:-4]}
  if cp.returncode:result['error']=cp.stderr[-350:]
  else:
   for check,config in [('target','scope_vector_insert_partial_oct15i.json'),
                        ('sibling','symbol_begin_scope_probe.json')]:
    try:result[check]=m.run(obj,R/'config'/config)
    except Exception as e:result[check]={'error':str(e)}
  rows.append(result)
  print(tag,'target',result.get('target',{}).get('exact_bytes'),
        result.get('target',{}).get('length'),
        'sibling_exact',result.get('sibling',{}).get('exact'),
        'error',result.get('error',''),flush=True)
(T/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
for snap in ('jan25','dec06'):
 kept=[(r['case'],r['target']['exact_bytes'],
        r['target']['different_offsets']) for r in rows
      if r['case'].startswith(snap+'-') and
      r.get('target',{}).get('length')==746 and
      r.get('sibling',{}).get('exact')]
 print('PROTECTED',snap,kept,flush=True)

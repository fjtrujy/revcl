#!/usr/bin/env python3
"""Probe historical i386 scheduling models on real SymbolTable TU and printer.

Nonproduction GCC controls, with unchanged genuine C++03 owners; validate the
original lengths/relocations and protect the exact member and printer startup.
"""
from pathlib import Path
import importlib.util,subprocess,json
ROOT=Path(__file__).resolve().parents[1]
OUT=Path('/tmp/vuc-sched-target-cpu-20261023')
OUT.mkdir(exist_ok=True)
sp=importlib.util.spec_from_file_location('vuc_compare_cpu', ROOT/'scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
models={
 'default':[],
 'mcpu-i386':['-mcpu=i386'],
 'mcpu-i486':['-mcpu=i486'],
 'mcpu-i586':['-mcpu=i586'],
 'mcpu-pentiumpro':['-mcpu=pentiumpro'],
 'mcpu-pentium3':['-mcpu=pentium3'],
 'mcpu-pentium4':['-mcpu=pentium4'],
 'mcpu-k6':['-mcpu=k6'],
 'mcpu-athlon':['-mcpu=athlon'],
 'mtune-i386':['-mtune=i386'],
 'mtune-i686':['-mtune=i686'],
 'mtune-pentium3':['-mtune=pentium3'],
 'march-i386':['-march=i386'],
 'march-i486':['-march=i486'],
 'march-i586':['-march=i586'],
 'march-i686':['-march=i686'],
}
owners={
 'vector':('probes/ScopeVectorInsertionOriginalUnitOct15i.cpp',
           ['-O2','-frename-registers','-falign-jumps=1'],
           [('target','scope_vector_insert_partial_oct15i.json'),('sibling','symbol_begin_scope_probe.json')]),
 'printer':('probes/PrintASTFunCallRealUnitOct15m.cpp',['-O2'],
           [('target','print_ast_fun_context_oct14_probe.json')]),
}
records=[]
for label,(source,base,checks) in owners.items():
 for snap in ('jan25','dec06'):
  for model,flags in models.items():
   tag=f'{label}-{snap}-{model}'
   obj=OUT/(tag+'.o')
   p=subprocess.run(['/tmp/vuc-binutils215/g++-'+snap+'-as215',*base,*flags,'-c',
           str(ROOT/source),'-o',str(obj)],text=True,capture_output=True)
   row={'name':tag,'flags':base+flags}
   if p.returncode: row['error']=p.stderr[-400:]
   else:
    for kind,cfg in checks:
     try:row[kind]=m.run(obj,ROOT/'config'/cfg)
     except Exception as e:row[kind]={'error':str(e)}
   records.append(row)
   if 'target' in row: print(tag,row['target'].get('exact_bytes'),row['target'].get('length'),
      row['target'].get('different_offsets',[])[:15],
      'sibling',row.get('sibling',{}).get('exact'),flush=True)
   else:print(tag,'FAIL',row.get('error','invalid'),flush=True)
(OUT/'results.json').write_text(json.dumps(records,indent=2)+'\n')
for owner in owners:
 for snap in ('jan25','dec06'):
  eligible=[r for r in records if r['name'].startswith(owner+'-'+snap+'-')
     and r.get('target',{}).get('exact_bytes') is not None
     and ('sibling' not in r or r['sibling'].get('exact'))]
  print('RANK',owner,snap,[(r['name'],r['target']['exact_bytes']) for r in
    sorted(eligible,key=lambda x:-x['target']['exact_bytes'])[:8]],flush=True)

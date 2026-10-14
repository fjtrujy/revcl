#!/usr/bin/env python3
"""Test VCL strong-owner (-fno-weak) transfer on actual near-exact VUC sources.

Requires original relocations/literals and ELF length, no padding, machine
patching or fabricated content. January and December GCC 3.3.3.
"""
import os,subprocess,importlib.util,collections
from pathlib import Path
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/vuc-near-exact-weak-controls-oct14');T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cases=[
  ('accum',R/'vuc/probes/ir_accumulatorable_joint_oct14.cpp',R/'vuc/config/ir_accumulatorable_probe.json',77,'j8'),
  ('printer',R/'vuc/probes/print_ast_fun_call.cpp',R/'vuc/config/print_ast_fun_call_probe.json',98,'plain'),
  ('tempC2',R/'vuc/probes/ml_temp_default.cpp',R/'vuc/config/ml_temp_default_C2_probe.json',101,'ren'),
  ('tempSet',R/'vuc/probes/ml_temp_set_return.cpp',R/'vuc/config/ml_temp_set_probe.json',117,'plain'),
  ('arena',R/'vuc/probes/instruction_list_arena_unit_oct14.cpp',R/'vuc/config/instruction_arena_cleanup_probe.json',46,'arena'),
]
default={'j8':['-O2','-frename-registers','-falign-jumps=8'],
'plain':['-O2'], 'ren':['-O2','-frename-registers'],
'arena':['-O2','-frename-registers','-fuse-cxa-atexit','-ffunction-sections']}
variants={
'baseline':[],
'no-weak':['-fno-weak'],
'weak+regmove':['-fno-weak','-fno-regmove'],
'weak+cost0':['-fno-weak','-mbranch-cost=0'],
'weak+guessoff':['-fno-weak','-fno-guess-branch-probability'],
'weak+gcseoff':['-fno-weak','-fno-gcse'],
'weak+sched2off':['-fno-weak','-fno-schedule-insns2'],
'weak+blockoff':['-fno-weak','-fno-reorder-blocks'],
'weak+loop1':['-fno-weak','-falign-loops=1'],
'weak+label1':['-fno-weak','-falign-labels=1'],
'weak+functionsections':['-fno-weak','-ffunction-sections'],
'weak+nofunctionsections':['-fno-weak','-fno-function-sections'],
}
compilers={'jan25':'/tmp/vuc-binutils215/g++-jan25-as215'}
if Path('/tmp/vuc-binutils215/g++-dec06-as215').is_file():
 compilers['dec06']='/tmp/vuc-binutils215/g++-dec06-as215'
if os.environ.get('VUC_SNAPSHOT'):
 compilers={k:v for k,v in compilers.items()
            if k==os.environ['VUC_SNAPSHOT']}
rank=[];sizes=collections.Counter()
for label,source,config,expected,flavor in cases:
 for snapshot,compiler in compilers.items():
  for mode,flags in variants.items():
   obj=T/(label+'-'+snapshot+'-'+mode+'.o')
   compileflags=default[flavor]+flags
   cp=subprocess.run([compiler,*compileflags,'-c',str(source),'-o',str(obj)],capture_output=True,text=True)
   if cp.returncode:
    print('COMPILE_ERROR',label,snapshot,mode,cp.stderr[-110:],flush=True);continue
   try:
    result=m.run(obj,config)
    rank.append((result['exact_bytes'],expected,label,snapshot,mode,result['different_offsets'][:12]))
    if result['exact']:
     print('NEW_EXACT',label,snapshot,mode,obj,flush=True);raise SystemExit(0)
   except ValueError as e:
    rank.append((0,expected,label,snapshot,mode,str(e)[:100]))
  print('DONE',label,snapshot,flush=True)
print('TOP')
for r in sorted(rank,reverse=True)[:35]:print(r,flush=True)

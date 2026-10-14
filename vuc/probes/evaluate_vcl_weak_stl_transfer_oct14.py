#!/usr/bin/env python3
"""Test VCL deferred-weak-owner lesson on *real* VUC GCC 3.3 STL templates.

VCL's latest main branch used -fno-weak in a verified source owner; that
is a hypothesis here, not a historical VUC Makefile assertion.
"""
from pathlib import Path
import subprocess,importlib.util,itertools,os
ROOT=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/vcl-weak-transfer-oct14');T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location('m',ROOT/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cases=[
 ('copy',ROOT/'vuc/probes/scope_uninitialized_copy.cpp',
  ROOT/'vuc/config/scope_uninitialized_copy_probe.json',
  '_ZSt24__uninitialized_copy_auxIP11MLHashTableI12MLGameStringP16SymbolTableEntry9MLHashFunIS1_ELi513EES7_ET0_T_S9_S8_12__false_type',120),
 ('reserve',ROOT/'vuc/probes/scope_vector_reserve_owner_oct14.cpp',
  ROOT/'vuc/config/scope_vector_reserve_oct14_probe.json',
  '_ZNSt6vectorI11MLHashTableI12MLGameStringP16SymbolTableEntry9MLHashFunIS1_ELi513EESaIS6_EE7reserveEj',241),
 ('symbolenter',ROOT/'vuc/src/SymbolTableUnit.cpp',
  ROOT/'vuc/config/symboltable_enter_oct13_probe.json',
  '_ZN11SymbolTable5EnterERK12MLGameStringP16SymbolTableEntry',185),
 ('symbolisolated',ROOT/'vuc/probes/symboltable_enter_oct13.cpp',
  ROOT/'vuc/config/symboltable_enter_oct13_probe.json',
  '_ZN11SymbolTable5EnterERK12MLGameStringP16SymbolTableEntry',185),
]
options={
'base':['-O2','-frename-registers'],
'no-weak':['-O2','-frename-registers','-fno-weak'],
'no-weak-j1':['-O2','-frename-registers','-fno-weak','-falign-jumps=1'],
'no-weak-j4':['-O2','-frename-registers','-fno-weak','-falign-jumps=4'],
'no-weak-loop1':['-O2','-frename-registers','-fno-weak','-falign-loops=1'],
'no-weak-label1':['-O2','-frename-registers','-fno-weak','-falign-labels=1'],
'no-weak-noreorder':['-O2','-frename-registers','-fno-weak','-fno-reorder-blocks'],
'no-weak-gcse':['-O2','-frename-registers','-fno-weak','-fno-gcse'],
'no-weak-branch0':['-O2','-frename-registers','-fno-weak','-mbranch-cost=0'],
'no-weak-noinline':['-O2','-frename-registers','-fno-weak','-fno-inline'],
'no-weak-plain':['-O2','-fno-weak'],
'no-weak-sections':['-O2','-frename-registers','-fno-weak','-ffunction-sections'],
}
rows=[]
if os.environ.get('VUC_TARGET'):
 cases=[case for case in cases if case[0]==os.environ['VUC_TARGET']]
for (name,source,config,symbol,target), (flag,opts) in itertools.product(cases,options.items()):
 obj=T/(name+'-'+flag+'.o')
 p=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(source),'-o',str(obj)],capture_output=True,text=True)
 if p.returncode:
  print('COMPILER_ERROR',name,flag,p.stderr[-150:],flush=True);continue
 record=next((x for x in subprocess.check_output(['nm','-S',str(obj)],text=True).splitlines() if x.split()[-1:]==[symbol]),'')
 size=int(record.split()[1],16) if record else -1
 if size!=target:
  rows.append((0,size,name,flag,'length'));continue
 try:
  score=m.run(obj,config)
  rows.append((score['exact_bytes'],size,name,flag,score['different_offsets'][:12]))
  if score['exact']:print('NEW_EXACT',name,flag,source,flush=True);raise SystemExit(0)
 except ValueError as e:rows.append((0,size,name,flag,'REJECT '+str(e)[:130]))
 print('CHECKED',name,flag,size,flush=True)
print('TOP')
for row in sorted(rows,reverse=True)[:25]:print(row,flush=True)

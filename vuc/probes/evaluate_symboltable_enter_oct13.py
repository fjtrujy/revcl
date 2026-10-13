#!/usr/bin/env python3
"""Try natural SymbolTable hash-node insertion source shapes under GCC 3.3.3."""
import importlib.util
import itertools
import subprocess
from pathlib import Path
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/symbol-enter-variants-oct13')
T.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location('matcher',R/'vuc/scripts/match_probe.py')
matcher=importlib.util.module_from_spec(spec);spec.loader.exec_module(matcher)
original=(R/'vuc/probes/symboltable_enter_oct13.cpp').read_text().replace('"../src/', '"'+str(R/'vuc/src')+'/')
access={
 'indexed': 'SymbolScope &scope = scopes_[scopes_.size() - 1];',
 'size-local': 'int last = scopes_.size() - 1; SymbolScope &scope = scopes_[last];',
 'back': 'SymbolScope &scope = scopes_.back();',
 'index-pointer': 'SymbolScope *scopeptr = &scopes_[scopes_.size() - 1]; SymbolScope &scope = *scopeptr;',
}
insert={
 'direct': 'node->next = (SymbolNode *)scope.heads[bucket];\n    scope.heads[bucket] = (SymbolScope::Node *)node;',
 'slot-ref': 'SymbolScope::Node *&slot = scope.heads[bucket];\n    node->next = (SymbolNode *)slot;\n    slot = (SymbolScope::Node *)node;',
 'slot-pointer': 'SymbolScope::Node **slot = &scope.heads[bucket];\n    node->next = (SymbolNode *)*slot;\n    *slot = (SymbolScope::Node *)node;',
 'old-local': 'SymbolNode *old = (SymbolNode *)scope.heads[bucket];\n    node->next = old;\n    scope.heads[bucket] = (SymbolScope::Node *)node;',
 'scope-first': 'scope.heads[bucket] = (SymbolScope::Node *)node;\n    node->next = (SymbolNode *)scope.heads[bucket];',
}
# The scope-first shape is intentionally excluded: it is NOT semantics-preserving.
insert.pop('scope-first')
flags={
 'o2':['-O2','-frename-registers'],
 'j4':['-O2','-frename-registers','-falign-jumps=4'],
 'j8':['-O2','-frename-registers','-falign-jumps=8','-falign-labels=1'],
 'j1':['-O2','-frename-registers','-falign-jumps=1'],
 'j4-nosched':['-O2','-frename-registers','-falign-jumps=4','-fno-schedule-insns2'],
 'j8-nosched':['-O2','-frename-registers','-falign-jumps=8','-fno-schedule-insns2'],
}
rank=[]
for (a,txt),(b,body) in itertools.product(access.items(),insert.items()):
 source=original.replace('SymbolScope &scope = scopes_[scopes_.size() - 1];',txt).replace('node->next = (SymbolNode *)scope.heads[bucket];\n    scope.heads[bucket] = (SymbolScope::Node *)node;',body)
 path=T/(a+'-'+b+'.cpp');path.write_text(source)
 for tag,fl in flags.items():
  obj=T/(a+'-'+b+'-'+tag+'.o')
  cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*fl,'-c',str(path),'-o',str(obj)],text=True,capture_output=True)
  if cp.returncode:
   rank.append((0,0,a,b,tag,cp.stderr[-100:]));continue
  syms=subprocess.run(['nm','-S',str(obj)],capture_output=True,text=True)
  sy=next((z.split()[1] for z in syms.stdout.splitlines() if '_ZN11SymbolTable5EnterERK' in z),None)
  size=int(sy,16) if sy else 0
  if size==185:
   try:
    r=matcher.run(obj,R/'vuc/config/symboltable_enter_oct13_probe.json')
    rank.append((r['exact_bytes'],size,a,b,tag,'VERIFIED'))
    if r['exact']:
     print('EXACT',a,b,tag,flush=True);raise SystemExit(0)
   except ValueError as e:rank.append((0,size,a,b,tag,str(e)[:100]))
  else:rank.append((0,size,a,b,tag,'length'))
 print('checked',a,b,flush=True)
for row in sorted(rank,reverse=True)[:32]:print(*row)

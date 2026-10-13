#!/usr/bin/env python3
"""Compile original SymbolTable methods in one authentic iostream source-unit context."""
import importlib.util
from pathlib import Path
import subprocess
R=Path(__file__).resolve().parents[2];T=Path('/tmp/vuc-binutils215/symboltable-unit-oct13');T.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location('matcher',R/'vuc/scripts/match_probe.py');matcher=importlib.util.module_from_spec(spec);spec.loader.exec_module(matcher)
raw=(R/'vuc/probes/symboltable_enter_oct13.cpp').read_text().replace('"../src/','"'+str(R/'vuc/src')+'/')
raw=raw.replace('#include <vector>','#include <vector>\n#include <iostream>')
raw=raw.replace('    void Enter(const MLGameString &name, SymbolTableEntry *entry);',
 '    void Enter(const MLGameString &name, SymbolTableEntry *entry);\n    SymbolTableEntry *Find(const MLGameString &name, bool localOnly);\n    void BeginScope();\n    void EndScope();')
enter=raw
find='''SymbolTableEntry *SymbolTable::Find(const MLGameString &name, bool localOnly) {
 for (int i = scopes_.size() - 1; i >= 0; --i) {
    SymbolTableEntry *result = scopes_[i].FindCopy(name);
    if (result) return result;
    if (localOnly) break;
 }
 return 0;
}
'''
begin='''void SymbolTable::BeginScope() {
 SymbolScope scope;
 scopes_.push_back(scope);
}
'''
end='''void SymbolTable::EndScope() { scopes_.pop_back(); }
'''
variants={
 'enter-only':'',
 'enter-find':find,
 'enter-find-begin':find+begin,
 'enter-find-begin-end':find+begin+end,
 'enter-begin-find':begin+find,
 'enter-begin-end-find':begin+end+find,
}
flags={
 'normal':['-O2','-frename-registers','-fuse-cxa-atexit','-ffunction-sections'],
 'normal-j1':['-O2','-frename-registers','-falign-jumps=1','-fuse-cxa-atexit','-ffunction-sections'],
 'normal-j8':['-O2','-frename-registers','-falign-jumps=8','-fuse-cxa-atexit','-ffunction-sections'],
 'plain':['-O2','-fuse-cxa-atexit','-ffunction-sections'],
}
for name,extra in variants.items():
 src=T/(name+'.cpp');src.write_text(enter+'\n'+extra)
 for opt,fl in flags.items():
  obj=T/(name+'-'+opt+'.o');cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*fl,'-c',str(src),'-o',str(obj)],text=True,capture_output=True)
  if cp.returncode:print(name,opt,'COMPILE_ERROR',cp.stderr[-180:],flush=True);continue
  try:
   r=matcher.run(obj,R/'vuc/config/iostream_global_symboltable_oct13_probe.json')
   print(name,opt,f"{r['exact_bytes']}/{r['length']}",flush=True)
   if r['exact']:
    print('EXACT',name,opt,flush=True);raise SystemExit(0)
  except ValueError as e:
   print(name,opt,'UNAVAILABLE',str(e)[:100],flush=True)

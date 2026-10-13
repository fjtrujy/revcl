#!/usr/bin/env python3
"""GCC 3.3.3 real Value.cpp source-unit context, preserving true function definitions."""
import importlib.util
import json
import subprocess
from pathlib import Path
R = Path(__file__).resolve().parents[2]
T = Path('/tmp/vuc-binutils215/value-full-oct13')
T.mkdir(parents=True, exist_ok=True)
matcher_spec=importlib.util.spec_from_file_location('matcher',R/'vuc/scripts/match_probe.py')
matcher=importlib.util.module_from_spec(matcher_spec); matcher_spec.loader.exec_module(matcher)
probe=R/'vuc/config/iostream_value_wrapper_oct12_probe.json'
raw=(R/'vuc/probes/value_mul_unit_ops_add_mul.cpp').read_text()
assert 'Value() : type(0)' in raw
raw=raw.replace('Value() : type(0),a(0),b(0),flag0(0),flag1(0),flag2(0),flag3(0) {}','Value();')
raw=raw.replace('Value Value::operator==', 'Value::Value() : type(0),a(0),b(0),flag0(0),flag1(0),flag2(0),flag3(0) {}\nValue Value::operator==',1)
# All sources are genuine reconstructed 2004 application methods, no dummy definitions.
groups = {
 'comparisons': raw[raw.index('Value Value::operator=='):raw.index('Value Value::operator+(')],
 'plus':raw[raw.index('Value Value::operator+('):raw.index('Value Value::operator*(')],
 'multiply':raw[raw.index('Value Value::operator*('):],
 'ctor':'Value::Value() : type(0),a(0),b(0),flag0(0),flag1(0),flag2(0),flag3(0) {}\n',
}
header='#include <iostream>\n'+raw[raw.index('struct Value {'):raw.index('Value::Value()')]
# The original constructor comes before operators, except variants specifically
# testing GCC unit context's effect on code-generation order.
sequences={
 'ctor-only':['ctor'],
 'ctor-compare':['ctor','comparisons'],
 'ctor-plus-compare':['ctor','plus','comparisons'],
 'ctor-plus-mul-compare':['ctor','plus','multiply','comparisons'],
 'ctor-plus-compare-mul':['ctor','plus','comparisons','multiply'],
 'ctor-compare-plus-mul':['ctor','comparisons','plus','multiply'],
 'ctor-mul-plus-compare':['ctor','multiply','plus','comparisons'],
 'ctor-mul-compare-plus':['ctor','multiply','comparisons','plus'],
 'ctor-compare-mul-plus':['ctor','comparisons','multiply','plus'],
 'ctor-compare-plus':['ctor','comparisons','plus'],
 'ctor-mul-compare':['ctor','multiply','comparisons'],
 'ctor-mul':['ctor','multiply'],
 'ctor-plus':['ctor','plus'],
}
opts={
 'r':['-O2','-frename-registers','-fuse-cxa-atexit','-ffunction-sections'],
 'rf':['-O2','-frename-registers','-ffast-math','-fuse-cxa-atexit','-ffunction-sections'],
 'rj':['-O2','-frename-registers','-falign-jumps=1','-fuse-cxa-atexit','-ffunction-sections'],
 'rfj':['-O2','-frename-registers','-ffast-math','-falign-jumps=1','-fuse-cxa-atexit','-ffunction-sections'],
}
rank=[]
for name, parts in sequences.items():
 source=T/(name+'.cpp')
 source.write_text(header + '\n'.join(groups[p] for p in parts))
 for label, flags in opts.items():
  obj=T/(name+'-'+label+'.o')
  cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*flags,'-c',str(source),'-o',str(obj)],capture_output=True,text=True)
  if cp.returncode:
   rank.append((0,name,label,cp.stderr[-130:]));continue
  try:
   result=matcher.run(obj,probe)
   rank.append((result['exact_bytes'],name,label,result['length']))
   if result['exact']:
    print('EXACT',name,label,flush=True)
    raise SystemExit(0)
  except ValueError as e:rank.append((0,name,label,str(e)[:110]))
 print('tested',name,flush=True)
for row in sorted(rank,reverse=True)[:24]:print(*row)

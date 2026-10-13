#!/usr/bin/env python3
"""Test genuine Value::operator/ lexical switch-arm order and vector forms.

Keep the recovered +,-,* predecessors, the correct one-relocation ABI
and a local scalar divisor (178/235 before this experiment). Only natural
C++ expressions and original case arm permutations are tested.
"""
import importlib.util
import itertools
import subprocess
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
TMP=Path('/tmp/vuc-binutils215/division-case-order-oct13')
TMP.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location('matcher',ROOT/'vuc/scripts/match_probe.py')
matcher=importlib.util.module_from_spec(spec);spec.loader.exec_module(matcher)
source=(ROOT/'vuc/probes/ValueDivisionUnit.cpp').read_text()
fn=source.index('Value Value::operator/(')
sw=source.index('switch (result.type)',fn)
begin=source.index('\n        case 1:',sw)
end=source.index('\n        }\n',begin)
original=source[begin:end]
a=original.index('\n        case 3:')
b=original.index('\n        case 4:')
arms={1:original[:a],3:original[a:b],4:original[b:]}
assert all('case '+str(k)+':' in v for k,v in arms.items())
variants={
 'baseline': '',
 'vector-first': '''            float a = 1.0f / temporary.values[0];
            float b = 1.0f / temporary.values[1];
            float c = 1.0f / temporary.values[2];
            float d = 1.0f / temporary.values[3];
            result.values[0] *= a;
            result.values[1] *= b;
            result.values[2] *= c;
            result.values[3] *= d;
''',
 'multiply-assignment': '''            result.values[0] = result.values[0] * (1.0f / temporary.values[0]);
            result.values[1] = result.values[1] * (1.0f / temporary.values[1]);
            result.values[2] = result.values[2] * (1.0f / temporary.values[2]);
            result.values[3] = result.values[3] * (1.0f / temporary.values[3]);
''',
 'vector-scalar-ops': '''            for (int i=0;i<4;++i)
                result.values[i] *= 1.0f / temporary.values[i];
''',
 'divide-vector': '''            result.values[0] /= temporary.values[0];
            result.values[1] /= temporary.values[1];
            result.values[2] /= temporary.values[2];
            result.values[3] /= temporary.values[3];
''',
}
scores=[]
for variant,vector in variants.items():
 for order in itertools.permutations((1,3,4)):
  localarms=dict(arms)
  if vector:
   assert localarms[4].count('result.values[0]')==1
   start=localarms[4].index('            result.values[0] *= 1.0f')
   finish=localarms[4].index('            break;',start)
   localarms[4]=localarms[4][:start]+vector+localarms[4][finish:]
   if 'float a =' in vector:
    localarms[4]=localarms[4].replace('case 4:', 'case 4: {',1).replace('            break;', '            break; }',1)
  candidate=source[:begin]+''.join(localarms[k] for k in order)+source[end:]
  path=TMP/(variant+'-'+''.join(str(k) for k in order)+'.cpp')
  path.write_text(candidate)
  for jump,extra in [(1,()),(4,()),(8,()),(8,('-fno-gcse',)),(8,('-fno-rename-registers',))]:
   obj=TMP/(path.stem+'-'+str(jump)+'-'+str(len(extra))+('.nr' if 'rename' in ''.join(extra) else '')+'.o')
   flags=['-O2','-frename-registers',f'-falign-jumps={jump}',*extra]
   cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*flags,'-c',str(path),'-o',str(obj)],capture_output=True,text=True)
   if cp.returncode:
    print('ERROR',path.name,jump,cp.stderr[-95:],flush=True);continue
   try:
    res=matcher.run(obj,ROOT/'vuc/config/value_division_partial_oct13.json')
    scores.append((res['exact_bytes'],res['length'],path.stem,flags,res['different_offsets'][:12]))
    if res['exact']:
     print('NEW_EXACT',path,flags,flush=True);raise SystemExit(0)
   except ValueError as e:
    pass
  print('TESTED',path.stem,flush=True)
print('BEST')
for row in sorted(scores,reverse=True)[:26]:print(row)

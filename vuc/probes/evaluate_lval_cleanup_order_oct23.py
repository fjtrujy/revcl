#!/usr/bin/env python3
"""Target the last original-length gap using actual cleanup and branch shapes.

Start from real AST child pointers consumed after push/new[] (verified 0x68
frame, 755 bytes), not dummy volatile data or forced hard registers.
Require real ELF reloc/literal verification for every original-size body.
"""
from pathlib import Path
import itertools,importlib.util,re,json,subprocess,hashlib
ROOT=Path(__file__).resolve().parents[1]
SCRATCH=Path('/tmp/vuc-ast-lval-cleanup-20261023');SCRATCH.mkdir(exist_ok=True)
INPUT=Path('/tmp/vuc-ast-lval-lifetimes-20261023')
sp=importlib.util.spec_from_file_location('auth',ROOT/'scripts/match_probe.py')
matcher=importlib.util.module_from_spec(sp);sp.loader.exec_module(matcher)
popforms={
 'old':'if (length > 0)\n            --length;',
 'local':'int count = length; if (count > 0) length = count - 1;',
 'nonzero':'if (length != 0)\n            --length;',
 'gte1':'if (length >= 1)\n            --length;',
 'post':'if (length > 0)\n            length--;',
 'local-post':'int current = length; if (current > 0) length = current - 1;',
 'signed-decrement':'if (length > 0)\n            length -= 1;',
}
cases={}
for baseline in ('count-after__both','count-after__both-member','test-equality__both'):
 f=INPUT/(baseline+'.cpp')
 if not f.exists(): continue
 source=f.read_text()
 marker='''    case 0: {'''
 start=source.index(marker)
 stop=source.index('\n    }\n}',start)
 casesraw=source[start:stop]
 members=[m.group() for m in re.finditer(r'    case [012]: \{.*?\n    \}',casesraw,re.S)]
 assert len(members)==3,(baseline,len(members))
 for order in itertools.permutations(range(3)):
  group='\n'.join(members[i] for i in order)
  cases[baseline+'__'+''.join(map(str,order))]=source.replace(casesraw,group)
for name,body in list(cases.items()):
 for popname,pop in popforms.items():
  old=popforms['old']
  assert old in body
  cases[name+'__'+popname]=body.replace(old,pop)
# Drop intermediate order-only bodies; only pop variants are needed.
cases={k:v for k,v in cases.items() if k.rsplit('__',1)[-1] in popforms}
print('CANDIDATES',len(cases),flush=True)
records=[]
for i,(name,source) in enumerate(cases.items()):
 p=SCRATCH/(name+'.cpp');p.write_text(source)
 record={'case':name,'source_sha256':hashlib.sha256(source.encode()).hexdigest()}
 for snap in ('jan25','dec06'):
  # Both historical snapshots matched in the 180-build earlier survey.
  # Re-test Dec06 for any Jan25 near-original-length candidate.
  if snap=='dec06' and record.get('jan25',{}).get('size',0)<760:
   record[snap]={'skipped':'Jan25 is below 760 bytes'}
   continue
  obj=SCRATCH/(name+'-'+snap+'.o')
  command=['/tmp/vuc-binutils215/g++-'+snap+'-as215','-O2','-c',str(p),'-o',str(obj)]
  result=subprocess.run(command,capture_output=True,text=True)
  if result.returncode:record[snap]={'error':result.stderr[-300:]};continue
  nm=subprocess.check_output(['nm','-S',str(obj)],text=True)
  m=next((x.split() for x in nm.splitlines()
   if x.endswith('_Z5PrintP11ASTLValExprRSo')),None)
  size=int(m[1],16) if m else 0
  entry={'size':size}
  if size==765:
   try:entry.update(matcher.run(obj,ROOT/'config/print_ast_lval_recursion_oct23_probe.json'))
   except Exception as exc:entry['match_error']=str(exc)
   print('ORIGINAL_LENGTH',name,snap,'match',entry.get('exact_bytes','rejected'),
         entry.get('match_error',''),flush=True)
  record[snap]=entry
 records.append(record)
 if len(records)%30==0:print('completed',len(records),flush=True)
(SCRATCH/'results.json').write_text(json.dumps(records,indent=2)+'\n')
from collections import Counter
for snap in ('jan25','dec06'):
 lengths=Counter(r[snap].get('size',0) for r in records if 'size' in r[snap])
 print('SUMMARY',snap,len(records),dict(sorted(lengths.items())),flush=True)

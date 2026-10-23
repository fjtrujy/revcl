#!/usr/bin/env python3
"""Rebuild the three genuine ASTLValExpr printer case bodies in natural orders.

This is a diagnostic *size/layout* survey. Wrong-size bodies cannot become
strict partials; only separately authenticated original-size outputs qualify.
Do not hand-assemble or pad missing code.
"""
from pathlib import Path
import itertools,re,subprocess,json,importlib.util
R=Path(__file__).resolve().parents[1]
out=Path('/tmp/vuc-lval-real-cases-20261023');out.mkdir(exist_ok=True)
src=(R/'probes/PrintASTLValRecursionOwnerOct23.cpp').read_text().replace(
    '#include "../src/MLGameString.h"',
    '#include "'+str(R/'src/MLGameString.h')+'"')
start=src.index('    case 0: {')
stop=src.index('\n    }\n}',start)
body=src[start:stop]
base=[m.group() for m in re.finditer(r'    case [012]: \{.*?\n    \}',body,re.S)]
assert len(base)==3
needle='    case 0: {'
assert src.count(needle)==1
order_case={}
for order in itertools.permutations(range(3)):
 source=src.replace(body,'\n'.join(base[i] for i in order))
 order_case[''.join(map(str,order))]=source
switchstart=src.index('    switch (node->lvalueKind) {')
for order in itertools.permutations(range(3)):
 chain=[]
 for position,caseid in enumerate(order):
  case=base[caseid].replace(f'    case {caseid}: {{',
          ('    if' if position==0 else '    else if') + f' (node->lvalueKind == {caseid}) {{')
  case=case.replace('        break;\n','')
  chain.append(case)
 order_case['if'+''.join(map(str,order))]=src[:switchstart]+'\n'.join(chain)+'\n}\n'
flags={
 'plain':['-O2'], 'rename':['-O2','-frename-registers'],
 'jump1':['-O2','-falign-jumps=1'],
 'jump8':['-O2','-falign-jumps=8'],
 'loop1':['-O2','-falign-loops=1'],
}
for key,source in order_case.items():
 (out/(key+'.cpp')).write_text(source)
allrows=[]
for compiler in ('jan25','dec06'):
 for order,source in order_case.items():
  first_row=len(allrows)
  for label,options in flags.items():
   if order.startswith('if') and label not in ('plain','rename','jump8'):
    continue
   tag=compiler+'-'+order+'-'+label
   obj=out/(tag+'.o')
   proc=subprocess.run(['/tmp/vuc-binutils215/g++-'+compiler+'-as215',*options,'-c',
        str(out/(order+'.cpp')),'-o',str(obj)],capture_output=True,text=True)
   row={'case':tag}
   if proc.returncode:row['error']=proc.stderr[-300:]
   else:
    nm=subprocess.check_output(['nm','-S',str(obj)],text=True)
    symbol=next((r.split() for r in nm.splitlines() if r.endswith('_Z5PrintP11ASTLValExprRSo')),None)
    row['length']=int(symbol[1],16) if symbol else 0
   allrows.append(row)
   if row.get('length')==765:print('ORIGINAL_LENGTH',tag,flush=True)
  print(compiler,order,[r.get('length',0) for r in allrows[first_row:]],flush=True)
(out/'results.json').write_text(json.dumps(allrows,indent=2)+'\n')
from collections import Counter
print('SIZES',dict(sorted(Counter(x.get('length',0) for x in allrows).items())),flush=True)

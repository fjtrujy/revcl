#!/usr/bin/env python3
"""Fresh source-shape controls for untouched Canonize_TryReduceExpr pair.

Compiler-visible original code: expression type 5 binary; type 6
unary/identifier; type 7 named constant. Both template bodies share
their 359-byte source shape. Try authentic C++03 success-result locals
and boolean return idioms, not optimizer option sweeps.
"""
from pathlib import Path
import itertools,subprocess,json,importlib.util
R=Path(__file__).resolve().parents[2]/'vuc'
O=Path('/tmp/vuc-binutils215/canonize-new-source-idioms-oct15p')
O.mkdir(exist_ok=True)
base=(R/'probes/CanonizeTryReduceNewOct15p.cpp').read_text()
for inc in ('MLHashTable.h','MLHashFunGameString.h'):
 base=base.replace('#include "../src/'+inc+'"', '#include "'+str(R/'src'/inc)+'"')
assert 'T accepted=0;' in base
forms={
 'T':base,
 'int':base.replace('T accepted=0;', 'int accepted=0;'),
 'bool':base.replace('T accepted=0;', 'bool accepted=false;'),
 'unsigned':base.replace('T accepted=0;', 'unsigned accepted=0;'),
 'flag-before':base.replace('T left=0,right=0;\n        T accepted=0;', 'T accepted=0;\n        T left=0,right=0;'),
 'flag-middle':base.replace('T left=0,right=0;\n        T accepted=0;', 'T left=0;\n        T accepted=0;\n        T right=0;'),
 'assignment-inline':base.replace('T accepted=0;', 'T accepted=0;').replace('if(Canonize_TryReduceExpr(expr->lhs,left) &&\n           Canonize_TryReduceExpr(expr->rhs,right) &&\n           ReduceBinOp(expr->op,left,right,out))\n            accepted=1;', 'if(Canonize_TryReduceExpr(expr->lhs,left))\n          if(Canonize_TryReduceExpr(expr->rhs,right))\n             accepted=ReduceBinOp(expr->op,left,right,out);'),
 'continue-block':base.replace('if(Canonize_TryReduceExpr(expr->lhs,left) &&\n           Canonize_TryReduceExpr(expr->rhs,right) &&\n           ReduceBinOp(expr->op,left,right,out))\n            accepted=1;', 'if(!Canonize_TryReduceExpr(expr->lhs,left)) return accepted;\n        if(!Canonize_TryReduceExpr(expr->rhs,right)) return accepted;\n        if(ReduceBinOp(expr->op,left,right,out)) accepted=1;'),
}
rows=[]
for name,source in forms.items():
 cpp=O/(name+'.cpp');cpp.write_text(source)
 for align in (1,2,4,8):
  obj=O/(name+'-j'+str(align)+'.o')
  p=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215','-O2','-frename-registers',f'-falign-jumps={align}','-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
  row={'name':name,'jump':align}
  if p.returncode:row['error']=p.stderr[-150:]
  else:
   nm=subprocess.check_output(['nm','-S','--defined-only',str(obj)],text=True)
   lengths=[int(x.split()[1],16) for x in nm.splitlines() if 'Canonize_TryReduceExpr' in x]
   row['lengths']=lengths
  rows.append(row)
  print(row,flush=True)
(O/'results.json').write_text(json.dumps(rows,indent=2)+'\n')

#!/usr/bin/env python3
"""First-source controls for untouched VUC ReduceBinOp<int/unsigned>.

Recover 14 actual original operation codes with valid C++03, test
conditional-chain/switch source and destination ownership, no assembly.
Only original-sized functions count as strict partial candidates.
"""
from pathlib import Path
import json,subprocess,importlib.util,itertools
ROOT=Path(__file__).resolve().parents[2]
R=ROOT/'vuc'; OUT=Path('/tmp/vuc-binutils215/reduce-binop-first-sources-oct15o')
OUT.mkdir(exist_ok=True)
spec=importlib.util.spec_from_file_location('m',R/'scripts/match_probe.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
src=(R/'probes/ReduceBinOpNewOct15o.cpp').read_text()
start=src.index('    unsigned operation=op->operation;')
end=src.index('\n}\n\ntemplate bool',start)
tests=[
 (13,'lhs+rhs'),(12,'lhs-rhs'),(11,'lhs*rhs'),(10,'lhs/rhs'),(7,'lhs==rhs'),
 (6,'lhs!=rhs'),(5,'lhs>rhs'),(4,'lhs<rhs'),(3,'lhs>=rhs'),(2,'lhs<=rhs'),
 (1,'lhs&&rhs'),(0,'lhs||rhs'),(9,'lhs&rhs'),(8,'lhs|rhs')]
def cb(expr,flip):
 if not flip:return expr
 return expr.replace('lhs','LEFT').replace('rhs','lhs').replace('LEFT','rhs')
variants={}
for flip,typ,assignment in itertools.product((False,True),('else-chain','switch','separate','switch-return'),('out','result')):
 if assignment=='result':
  prologue='    T result=0;\n'
  put=lambda e: 'result = '+e+';'
  epilogue='    out=result;\n    return true;'
 else:
  prologue=''
  put=lambda e: 'out = '+e+';'
  epilogue='    return true;'
 if typ=='else-chain':
  statement='\n'.join('    '+('if' if i==0 else 'else if')+' (op->operation==%d) '%n+put(cb(e,flip)) for i,(n,e) in enumerate(tests))
  statement+='\n    else return false;\n'+epilogue
 elif typ=='separate':
  statement='\n'.join('    if (op->operation==%d) { %s return true; }'%(n,put(cb(e,flip))) for n,e in tests)
  statement+='\n    return false;'
 elif typ=='switch':
  statement='    switch(op->operation) {\n'+'\n'.join('    case %d: %s break;'%(n,put(cb(e,flip))) for n,e in tests)+'\n    default: return false;\n    }\n'+epilogue
 else:
  statement='    switch(op->operation) {\n'+'\n'.join('    case %d: %s return true;'%(n,put(cb(e,flip))) for n,e in tests)+'\n    default: return false;\n    }'
 variants[f'{typ}-{assignment}-'+('rhs-first' if flip else 'lhs-first')]=prologue+statement
rows=[]
for name,body in variants.items():
 cpp=OUT/(name+'.cpp');cpp.write_text(src[:start]+body+src[end:])
 obj=OUT/(name+'.o')
 cc=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215','-O2','-frename-registers','-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 row={'case':name}
 if cc.returncode:row['error']=cc.stderr[-170:]
 else:
  symbols=subprocess.check_output(['nm','-S','--defined-only',str(obj)],text=True)
  for signed,mangled in [('unsigned','_Z11ReduceBinOpIjEbPN2IR13Operator_DataERKT_S5_RS3_'),('signed','_Z11ReduceBinOpIiEbPN2IR13Operator_DataERKT_S5_RS3_')]:
   sym=next((line.split() for line in symbols.splitlines() if line.endswith(mangled)),None)
   row[signed+'-length']=int(sym[1],16) if sym else -1
   if sym and int(sym[1],16)==315:
    cfg={'function':mangled,'address':'0x08050540' if signed=='unsigned' else '0x08050680','size':315,'source':'vuc/probes/ReduceBinOpNewOct15o.cpp','flags':['-O2','-frename-registers'],'relocations':{}}
    path=OUT/(signed+'-config.json');path.write_text(json.dumps(cfg))
    try:
     v=m.run(obj,path)
     row[signed+'-matching']=v['exact_bytes']
     row[signed+'-differences']=v['different_offsets'][:22]
    except ValueError as e:row[signed+'-error']=str(e)
 rows.append(row)
 print(row,flush=True)
(OUT/'results.json').write_text(json.dumps(rows,indent=2)+'\n')

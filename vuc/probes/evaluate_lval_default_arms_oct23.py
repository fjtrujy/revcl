#!/usr/bin/env python3
"""Try explicit no-op/default selector arms grounded in original LValue return.

The original returns for all unknown discriminants (including negative).
These extra no-op arms are semantically equivalent while affecting early GCC
jump partitioning, rather than artificial NOP insertion.
"""
from pathlib import Path
import subprocess,json,importlib.util
R=Path(__file__).resolve().parents[1]
O=Path('/tmp/vuc-lval-default-arms-20261023');O.mkdir(exist_ok=True)
spec=importlib.util.spec_from_file_location('m',R/'scripts/match_probe.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
source=Path('/tmp/vuc-ast-lval-lifetimes-20261023')
variants={}
extra={
 'default-return':'    default: return;',
 'default-break':'    default: break;',
 'minus1-return':'    case -1: return;',
 'minus1-break':'    case -1: break;',
 'three-return':'    case 3: return;',
 'three-break':'    case 3: break;',
 'zero-case-first':'    case -1: return;\n    default: return;',
 'far-case':'    case -2: return;\n    default: return;',
}
for family in ('count-after__both','count-after__both-member','test-equality__both','copy-prestore__both'):
 text=(source/(family+'.cpp')).read_text()
 fragment='''    }
    }
}'''
 assert text.endswith(fragment+'\n')
 for label,addition in extra.items():
  updated=text[:-len(fragment+'\n')]+'''    }
'''+addition+'''
    }
}
'''
  variants[family+'__'+label]=updated
result=[]
for tag,code in variants.items():
 f=O/(tag+'.cpp');f.write_text(code)
 row={'case':tag}
 for snap in ('jan25','dec06'):
  if snap=='dec06' and row.get('jan25',{}).get('size',0)<760:continue
  obj=O/(tag+'-'+snap+'.o')
  cp=subprocess.run(['/tmp/vuc-binutils215/g++-'+snap+'-as215','-O2','-c',str(f),'-o',str(obj)],capture_output=True,text=True)
  if cp.returncode:row[snap]={'error':cp.stderr[-400:]};continue
  sym=next((x.split() for x in subprocess.check_output(['nm','-S',str(obj)],text=True).splitlines() if x.endswith('_Z5PrintP11ASTLValExprRSo')),None)
  size=int(sym[1],16) if sym else 0
  item={'size':size}
  if size==765:
   try:item.update(m.run(obj,R/'config/print_ast_lval_recursion_oct23_probe.json'))
   except Exception as e:item['match_error']=str(e)
   print('ORIGINAL_SIZE',tag,snap,item.get('exact_bytes','REJECT'),item.get('match_error',''),flush=True)
  row[snap]=item
 result.append(row)
 print(tag,row.get('jan25',{}).get('size'),row.get('dec06',{}).get('size'),flush=True)
(O/'results.json').write_text(json.dumps(result,indent=2)+'\n')

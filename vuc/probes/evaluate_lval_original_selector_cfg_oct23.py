#!/usr/bin/env python3
"""Recreate genuine ASTLVal selector CFG from original pair of test instructions.

At 0x805c509 the original tests kind==0 and at 0x805c511 tests kind<=0.
Model these as C++03 source predicates with genuine recursive bodies.
Only claim matches if symbol size, all ELF relocations and literal slices pass.
"""
from pathlib import Path
import itertools,re,json,subprocess,importlib.util
R=Path(__file__).resolve().parents[1]
O=Path('/tmp/vuc-lval-kind-cfg-20261023');O.mkdir(exist_ok=True)
inpath=Path('/tmp/vuc-ast-lval-lifetimes-20261023')
sp=importlib.util.spec_from_file_location('matcher',R/'scripts/match_probe.py')
matcher=importlib.util.module_from_spec(sp);sp.loader.exec_module(matcher)
modes=['nested','nested-reverse','positive-first','negative-first','single-local','double-guard']
sources={}
for family in ('count-after__both','count-after__both-member','test-equality__both','copy-prestore__both'):
 raw=(inpath/(family+'.cpp')).read_text()
 start=raw.index('    switch (node->lvalueKind) {')
 end=raw.index('\n    }\n}',start)
 chunks=dict((int(i),text.replace('        break;',''))
      for i,text in re.findall(r'    case ([012]): \{(.*?)\n    \}',raw[start:end],re.S))
 assert len(chunks)==3
 def ifcase(i,expr,keyword='if'):
  return f'    {keyword} ({expr}) {{'+chunks[i]+'\n    }'
 for mode in modes:
  before=raw[:start]
  after=raw[end+6:]
  if mode=='nested':
   branches='''    int kind = node->lvalueKind;
    if (kind == 0) {'''+chunks[0]+'''
    } else if (kind > 0) {
       if (kind == 1) {'''+chunks[1]+'''
       } else if (kind == 2) {'''+chunks[2]+'''
       }
    }'''
  elif mode=='nested-reverse':
   branches='''    int kind = node->lvalueKind;
    if (kind != 0) {
       if (kind > 0) {
         if (kind == 1) {'''+chunks[1]+'''
         } else if (kind == 2) {'''+chunks[2]+'''
         }
       }
    } else {'''+chunks[0]+'''
    }'''
  elif mode=='positive-first':
   branches='''    int kind = node->lvalueKind;
    if (kind > 0) {
       if (kind == 1) {'''+chunks[1]+'''
       } else if (kind == 2) {'''+chunks[2]+'''
       }
    } else if (kind == 0) {'''+chunks[0]+'''
    }'''
  elif mode=='negative-first':
   branches='''    int kind = node->lvalueKind;
    if (kind < 0) return;
    if (kind == 0) {'''+chunks[0]+'''
    } else if (kind == 1) {'''+chunks[1]+'''
    } else if (kind == 2) {'''+chunks[2]+'''
    }'''
  elif mode=='single-local':
   branches='''    int kind = node->lvalueKind;
    if (!kind) {'''+chunks[0]+'''
    } else if (kind == 1) {'''+chunks[1]+'''
    } else if (kind == 2) {'''+chunks[2]+'''
    }'''
  elif mode=='double-guard':
   branches='''    int kind = node->lvalueKind;
    if (kind <= 0) {
      if (kind == 0) {'''+chunks[0]+'''
      }
    } else if (kind == 1) {'''+chunks[1]+'''
    } else if (kind == 2) {'''+chunks[2]+'''
    }'''
  for st in ('lval','node'):
   candidate=before+branches+after
   if st=='node':
    candidate=candidate.replace('ASTLValExpr **','ASTNode **').replace(
      'new ASTLValExpr *[','new ASTNode *[').replace('sizeof(ASTLValExpr *)','sizeof(ASTNode *)').replace(
       'void Add(ASTLValExpr *node)','void Add(ASTNode *node)')
   sources[family+'__'+mode+'__'+st]=candidate
rows=[]
for tag,source in sources.items():
 file=O/(tag+'.cpp');file.write_text(source)
 row={'case':tag}
 for compiler in ('jan25','dec06'):
  if compiler=='dec06' and row.get('jan25',{}).get('size',0)<760:
   continue
  obj=O/(tag+'-'+compiler+'.o')
  proc=subprocess.run(['/tmp/vuc-binutils215/g++-'+compiler+'-as215','-O2','-c',str(file),'-o',str(obj)],
      capture_output=True,text=True)
  if proc.returncode:row[compiler]={'error':proc.stderr[-320:]};continue
  sym=next((s.split() for s in subprocess.check_output(['nm','-S',str(obj)],text=True).splitlines()
       if s.endswith('_Z5PrintP11ASTLValExprRSo')),None)
  size=int(sym[1],16) if sym else 0
  info={'size':size}
  if size==765:
   try:info.update(matcher.run(obj,R/'config/print_ast_lval_recursion_oct23_probe.json'))
   except Exception as e:info['match_error']=str(e)
   print('ORIGINAL_LENGTH',tag,compiler,'equal bytes',info.get('exact_bytes','REJECT'),
      info.get('match_error',''),flush=True)
  row[compiler]=info
 rows.append(row)
 print(tag,row.get('jan25',{}).get('size'),row.get('dec06',{}).get('size'),flush=True)
(O/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
from collections import Counter
print('SIZES',dict(sorted(Counter(x['jan25'].get('size',0) for x in rows).items())),flush=True)

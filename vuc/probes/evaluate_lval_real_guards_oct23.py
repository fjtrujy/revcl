#!/usr/bin/env python3
"""Small follow-on scan: real LValue child/name/literal lifetimes and dispatch CFG.

Use strongest genuine-count-update cases only. Candidate metadata remains in
scratch; only score original-length, authentic-reloc original-literal bodies.
"""
from pathlib import Path
import subprocess,importlib.util,re,json
R=Path(__file__).resolve().parents[1]
WORK=Path('/tmp/vuc-ast-lval-real-guards-20261023');WORK.mkdir(exist_ok=True)
ROOT=Path('/tmp/vuc-ast-lval-lifetimes-20261023')
sp=importlib.util.spec_from_file_location('m',R/'scripts/match_probe.py');m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
rows=[]
for base in ['count-after__both','count-after__both-member']:
 src=(ROOT/(base+'.cpp')).read_text()
 assert src.count('        ScopedPrintNode entry(node);')==3
 names={}
 needle='''        ScopedPrintNode entry(node);
        output << static_cast<ASTLValIDExpr *>(node)->name.c_str();'''
 assert needle in src
 names['id-ptr']=src.replace(needle,'''        MLGameString *name = &static_cast<ASTLValIDExpr *>(node)->name;
        ScopedPrintNode entry(node);
        output << name->c_str();''')
 names['id-ref']=src.replace(needle,'''        MLGameString &name = static_cast<ASTLValIDExpr *>(node)->name;
        ScopedPrintNode entry(node);
        output << name.c_str();''')
 names['id-constptr']=src.replace(needle,'''        MLGameString * const name = &static_cast<ASTLValIDExpr *>(node)->name;
        ScopedPrintNode entry(node);
        output << name->c_str();''')
 names['id-outstream']=src.replace(needle,'''        std::ostream * const stream = &output;
        ScopedPrintNode entry(node);
        *stream << static_cast<ASTLValIDExpr *>(node)->name.c_str();''')
 names['dot-pre']=src.replace('''        ScopedPrintNode entry(node);
        Print(memberBase, output);
        output << ".";''','''        const char *separator = ".";
        ScopedPrintNode entry(node);
        Print(memberBase, output);
        output << separator;''')
 names['bracket-pre']=src.replace('''        ScopedPrintNode entry(node);
        Print(arrayBase, output);
        output << "[";''','''        const char *open = "[";
        const char *close = "]";
        ScopedPrintNode entry(node);
        Print(arrayBase, output);
        output << open;''').replace('''        Print(arrayIndex, output);
        output << "]";''','''        Print(arrayIndex, output);
        output << close;''')
 # Source structure closer to the original's two tests and >0 guard.
 modes=[
  ('if0-first', (0,1,2)),
  ('if2-first', (2,1,0)),
  ('if1-first', (1,2,0)),
 ]
 marker='    switch (node->lvalueKind) {'
 start=src.index(marker); end=src.index('\n    }\n}',start)
 raw=src[start+len(marker):end]
 pieces=dict((int(n),contents) for n,contents in re.findall(r'    case ([012]): \{(.*?)\n    \}',raw,re.S))
 assert set(pieces)=={0,1,2}
 for shape,order in modes:
  out=[f'    {"if" if i==0 else "else if"} (node->lvalueKind == {k}) {{'+pieces[k].replace('        break;','')+'\n    }' for i,k in enumerate(order)]
  names[shape]=src[:start]+'\n'.join(out)+src[end+6:]
 for label,code in names.items():
  tag=base+'__'+label
  p=WORK/(tag+'.cpp');p.write_text(code)
  row={'case':tag}
  for snapshot in ('jan25','dec06'):
   if snapshot=='dec06' and row.get('jan25',{}).get('size',0)<755: continue
   obj=WORK/(tag+'-'+snapshot+'.o')
   cp=subprocess.run(['/tmp/vuc-binutils215/g++-'+snapshot+'-as215','-O2','-c',str(p),'-o',str(obj)],
       capture_output=True,text=True)
   if cp.returncode:row[snapshot]={'error':cp.stderr[-500:]}; continue
   nm=subprocess.check_output(['nm','-S',str(obj)],text=True)
   sym=next((x.split() for x in nm.splitlines() if x.endswith('_Z5PrintP11ASTLValExprRSo')),None)
   size=int(sym[1],16) if sym else 0
   entry={'size':size}
   if size==765:
    try:entry.update(m.run(obj,R/'config/print_ast_lval_recursion_oct23_probe.json'))
    except Exception as exc:entry['match_error']=str(exc)
    print('ORIGINAL_LENGTH',tag,snapshot,'byte-match',entry.get('exact_bytes','REJECT'),entry.get('match_error',''),flush=True)
   row[snapshot]=entry
  print(tag,row.get('jan25',{}).get('size'),row.get('dec06',{}).get('size'),flush=True)
  rows.append(row)
(WORK/'results.json').write_text(json.dumps(rows,indent=2)+'\n')

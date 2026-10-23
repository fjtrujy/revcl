#!/usr/bin/env python3
"""Authentic C++03 scope-ownership controls for ASTLValExpr printer."""
from pathlib import Path
import subprocess,json,importlib.util
R=Path(__file__).resolve().parents[1]
OUT=Path('/tmp/vuc-lval-owner-guard-20261023');OUT.mkdir(exist_ok=True)
spec=importlib.util.spec_from_file_location('checker',R/'scripts/match_probe.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
old='''class ScopedPrintNode {
public:
    ScopedPrintNode(ASTLValExpr *node) { matchStack.Add(node); }
    ~ScopedPrintNode() { matchStack.Pop(); }
};'''
models={
 'baseline':old,
 'stack-pointer':'''class ScopedPrintNode {
    MatchStack *stack;
public:
    ScopedPrintNode(ASTLValExpr *node) : stack(&matchStack) { stack->Add(node); }
    ~ScopedPrintNode() { stack->Pop(); }
};''',
 'stack-ref':'''class ScopedPrintNode {
    MatchStack &stack;
public:
    ScopedPrintNode(ASTLValExpr *node) : stack(matchStack) { stack.Add(node); }
    ~ScopedPrintNode() { stack.Pop(); }
};''',
 'node-owned':'''class ScopedPrintNode {
    ASTLValExpr *entered;
public:
    ScopedPrintNode(ASTLValExpr *node) : entered(node) { matchStack.Add(entered); }
    ~ScopedPrintNode() { if (entered) matchStack.Pop(); }
};''',
 'stack-ref-node':'''class ScopedPrintNode {
    MatchStack &stack;
    ASTLValExpr *entered;
public:
    ScopedPrintNode(ASTLValExpr *node) : stack(matchStack), entered(node) { stack.Add(entered); }
    ~ScopedPrintNode() { if (entered) stack.Pop(); }
};'''
}
forms={
 'count-after__both':Path('/tmp/vuc-ast-lval-lifetimes-20261023/count-after__both.cpp'),
 'count-after__both-member':Path('/tmp/vuc-ast-lval-lifetimes-20261023/count-after__both-member.cpp'),
 'test-equality__both__nested-reverse':Path('/tmp/vuc-lval-kind-cfg-20261023/test-equality__both__nested-reverse__node.cpp'),
 'copy-prestore__both__negative-first':Path('/tmp/vuc-lval-kind-cfg-20261023/copy-prestore__both__negative-first__node.cpp'),
}
variants={}
for basename,path in forms.items():
 src=path.read_text()
 assert old in src
 for model,new in models.items():
  for mode in ('original','node'):
   candidate=src.replace(old,new)
   if mode=='node':
    candidate=candidate.replace('ASTLValExpr **','ASTNode **').replace(
      'new ASTLValExpr *[','new ASTNode *[').replace(
      'sizeof(ASTLValExpr *)','sizeof(ASTNode *)').replace(
      'void Add(ASTLValExpr *node)','void Add(ASTNode *node)')
   variants[basename+'__'+model+'__'+mode]=candidate
print('VARIANTS',len(variants),flush=True)
entries=[]
for tag,source in variants.items():
 file=OUT/(tag+'.cpp');file.write_text(source)
 row={'case':tag}
 for snap in ('jan25','dec06'):
  if snap=='dec06' and row.get('jan25',{}).get('size',0)<760: continue
  obj=OUT/(tag+'-'+snap+'.o')
  result=subprocess.run(['/tmp/vuc-binutils215/g++-'+snap+'-as215','-O2','-c',str(file),'-o',str(obj)],capture_output=True,text=True)
  if result.returncode:row[snap]={'error':result.stderr[-350:]};continue
  line=next((x.split() for x in subprocess.check_output(['nm','-S',str(obj)],text=True).splitlines()
    if x.endswith('_Z5PrintP11ASTLValExprRSo')),None)
  n=int(line[1],16) if line else 0
  val={'size':n}
  if n==765:
   try:val.update(m.run(obj,R/'config/print_ast_lval_recursion_oct23_probe.json'))
   except Exception as e:val['match_error']=str(e)
   print('ORIGINAL_LENGTH',tag,snap,val.get('exact_bytes','REJECT'),val.get('match_error',''),flush=True)
  row[snap]=val
 entries.append(row)
 print(tag,row.get('jan25',{}).get('size'),row.get('dec06',{}).get('size'),flush=True)
(OUT/'results.json').write_text(json.dumps(entries,indent=2)+'\n')

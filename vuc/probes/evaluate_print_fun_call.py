#!/usr/bin/env python3
"""Source-order sweeps for ASTFunCall printer with all literals authenticated."""
from pathlib import Path
import importlib.util,subprocess
R=Path(__file__).resolve().parents[2]; T=Path('/tmp/vuc-binutils215')
spec=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
source=(R/'vuc/probes/print_ast_fun_call.cpp').read_text()
block='''    Print(call->callee, output);
    output << "(";
    Print(call->arguments, output, ", ");
    output << ")";'''
assert block in source
forms={
'direct':block,
'callee-local':'''    ASTLValExpr *callee=call->callee;
    Print(callee, output);
    output << "(";
    Print(call->arguments, output, ", ");
    output << ")";''',
'stream-local':'''    std::ostream &out=output;
    Print(call->callee, out);
    out << "(";
    Print(call->arguments, out, ", ");
    out << ")";''',
'args-local':'''    Print(call->callee, output);
    output << "(";
    ASTNodeList<ASTExpr *> &args=call->arguments;
    Print(args, output, ", ");
    output << ")";''',
'three-locals':'''    ASTLValExpr *callee=call->callee;
    ASTNodeList<ASTExpr *> &args=call->arguments;
    Print(callee, output);
    output << "(";
    Print(args, output, ", ");
    output << ")";''',
'output-locals':'''    Print(call->callee, output);
    const char *lparen="(";
    output << lparen;
    const char *comma=", ";
    Print(call->arguments, output, comma);
    const char *rparen=")";
    output << rparen;''',
'pointer-to-stream':'''    std::ostream *out=&output;
    Print(call->callee, *out);
    *out << "(";
    Print(call->arguments, *out, ", ");
    *out << ")";''',
'direct-operator':'''    Print(call->callee, output);
    std::operator<<(output,"(");
    Print(call->arguments, output, ", ");
    std::operator<<(output,")");''',
'args-pointer':'''    Print(call->callee, output);
    output << "(";
    ASTNodeList<ASTExpr *> *args=&call->arguments;
    Print(*args, output, ", ");
    output << ")";''',
'paren-before':'''    ASTNodeList<ASTExpr *> *args=&call->arguments;
    Print(call->callee, output);
    output << "(";
    Print(*args, output, ", ");
    output << ")";''',
'lambda-free':'''    ASTLValExpr *expr=call->callee;
    Print(expr, output);
    output << "(";
    Print(call->arguments, output, ", ");
    output << ")";''',
}
flags={
'ren':['-O2','-frename-registers'],
'plain':['-O2'],
'ren-j1':['-O2','-frename-registers','-falign-jumps=1'],
'ren-noalias':['-O2','-frename-registers','-fno-strict-aliasing'],
}
results=[]
for name,code in forms.items():
 p=T/('funccall-'+name+'.cpp');p.write_text(source.replace(block,code).replace('../src/ASTNodeList.h','ASTNodeList.h'))
 for fl,opts in flags.items():
  obj=T/('funccall-'+name+'-'+fl+'.o')
  cp=subprocess.run([str(T/'g++-jan25-as215'),*opts,'-I',str(R/'vuc/src'),'-c',str(p),'-o',str(obj)],capture_output=True,text=True)
  if cp.returncode:results.append((0,name,fl,cp.stderr[-70:]));continue
  try:
   score=m.run(obj,R/'vuc/config/print_ast_fun_call_probe.json')
   results.append((score['exact_bytes'],name,fl,str(score['length'])))
  except Exception as e:results.append((0,name,fl,str(e)[:60]))
results.sort(reverse=True)
for row in results[:28]:print(*row)

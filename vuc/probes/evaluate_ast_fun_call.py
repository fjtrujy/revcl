#!/usr/bin/env python3
"""Sweep ordinary C++ expression ordering in VUC ASTFunCall Print().
No instruction injections or manual register selection.
"""
import importlib.util
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TMP = Path("/tmp/vuc-binutils215")
spec = importlib.util.spec_from_file_location("vucmatch",ROOT/"vuc/scripts/match_probe.py")
matcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matcher)
base = (ROOT/"vuc/probes/print_ast_fun_call.cpp").read_text()
start = base.index("void Print(ASTFunCall *call,")
header = base[:start]
forms = {
 "original":'Print(call->callee, output);\n    output << "(";\n    Print(call->arguments, output, ", ");\n    output << ")";',
 "load-callee":'ASTLValExpr *callee = call->callee;\n    Print(callee, output);\n    output << "(";\n    Print(call->arguments, output, ", ");\n    output << ")";',
 "load-args":'Print(call->callee, output);\n    output << "(";\n    ASTNodeList<ASTExpr *> &args=call->arguments;\n    Print(args, output, ", ");\n    output << ")";',
 "load-both":'ASTLValExpr *callee = call->callee;\n    Print(callee, output);\n    output << "(";\n    ASTNodeList<ASTExpr *> &args=call->arguments;\n    Print(args, output, ", ");\n    output << ")";',
 "return-local":'Print(call->callee, output);\n    output << "(";\n    Print(call->arguments, output, ", ");\n    std::ostream &out=output;\n    out << ")";',
 "separators":'Print(call->callee, output);\n    const char *open = "(";\n    output << open;\n    const char *sep=", ";\n    Print(call->arguments, output, sep);\n    const char *close=")";\n    output << close;',
 "literal-global":'Print(call->callee, output);\n    output.operator<<("(");\n    Print(call->arguments, output, ", ");\n    output.operator<<(")");',
 "args-ptr":'Print(call->callee, output);\n    output << "(";\n    ASTNodeList<ASTExpr *> *args=&call->arguments;\n    Print(*args, output, ", ");\n    output << ")";',
 "typed-ptr":'ASTFunCall *current=call;\n    Print(current->callee, output);\n    output << "(";\n    Print(current->arguments, output, ", ");\n    output << ")";',
 "callee-after":'ASTNodeList<ASTExpr *> &args=call->arguments;\n    Print(call->callee, output);\n    output << "(";\n    Print(args, output, ", ");\n    output << ")";',
 "pointer-first":'ASTNodeList<ASTExpr *> *args=&call->arguments;\n    ASTLValExpr *target=call->callee;\n    Print(target, output);\n    output << "(";\n    Print(*args, output, ", ");\n    output << ")";',
 "const-literal":'Print(call->callee, output);\n    const char *first="(";\n    output << first;\n    Print(call->arguments, output, ", ");\n    output << ")";',
 "static-literals":'Print(call->callee, output);\n    const char *first="(";\n    const char *middle=", ";\n    const char *last=")";\n    output << first;\n    Print(call->arguments, output, middle);\n    output << last;',
 "paren-object":'Print(call->callee, output);\n    (output << "(");\n    Print(call->arguments, output, ", ");\n    (output << ")");',
 "buffered-output":'std::ostream *stream=&output;\n    Print(call->callee, *stream);\n    *stream << "(";\n    Print(call->arguments, *stream, ", ");\n    *stream << ")";'
}
opts={
 "ren":["-O2","-frename-registers"],
 "plain":["-O2"],
 "ren-j1":["-O2","-frename-registers","-falign-jumps=1"],
 "ren-nocse":["-O2","-frename-registers","-fno-cse-follow-jumps"],
 "ren-nopeephole":["-O2","-frename-registers","-fno-peephole2"],
}
scores=[]
for name,body in forms.items():
 p=TMP/("funprint-"+name+".cpp")
 p.write_text(header+"void Print(ASTFunCall *call, std::ostream &output)\n{\n    "+body+"\n}\n")
 for option,flags in opts.items():
  obj=TMP/("funprint-"+name+"-"+option+".o")
  cp=subprocess.run([str(TMP/"g++-jan25-as215"),*flags,"-I",str(ROOT/"vuc/src"),"-c",str(p),"-o",str(obj)],capture_output=True,text=True)
  if cp.returncode:scores.append((0,name,option,"compile: "+cp.stderr[-70:]));continue
  try:
   result=matcher.run(obj,ROOT/"vuc/config/print_ast_fun_call_probe.json")
   scores.append((result["exact_bytes"],name,option,str(result["length"])))
  except Exception as e:scores.append((0,name,option,str(e)[:60]))
scores.sort(reverse=True)
for item in scores[:30]:print(*item)

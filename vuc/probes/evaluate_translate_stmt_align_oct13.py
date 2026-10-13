#!/usr/bin/env python3
"""Original-length GCC 3.3 translation-loop recovery with IR Seq_Data.

Test ordinary C++ AST list traversal and historical loop/label alignment.
Only accept complete relocation-authenticated 119-byte original function.
"""
from pathlib import Path
import subprocess,importlib.util,itertools
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/translate-stmt-align-oct13");T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location("m",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
src=(R/"vuc/probes/translate_stmt_list.cpp").read_text().replace('"../src/ASTNodeList.h"','"'+str(R/"vuc/src/ASTNodeList.h")+'"')
lo=src.index("    IR::Stmt_Data *result = 0;")
hi=src.index("    return result;",lo)+len("    return result;")
base=src[lo:hi]
forms={
"original":base,
"continue":"""    IR::Stmt_Data *result = 0;
    for (int i=0; i<statements.length; ++i) {
        IR::Stmt_Data *item=Translate(statements.entries[i]);
        if (!item) continue;
        if (!result) result=item;
        else result=(IR::Stmt_Data*)new IR::Seq_Data(result,item);
    }
    return result;""",
"invert":base.replace("if (result)\n                result=(IR::Stmt_Data *)new IR::Seq_Data(result, item);\n            else\n                result=item;",
                    "if (!result) result=item;\n            else result=(IR::Stmt_Data *)new IR::Seq_Data(result,item);"),
"index-increment":base.replace("for (int i=0; i<statements.length; ++i)","for (int i=0; i<statements.length; i++)"),
"while":base.replace("for (int i=0; i<statements.length; ++i) {","int i = 0;\n    while (i < statements.length) {\n        ++i;").replace("statements.entries[i]","statements.entries[i - 1]"),
}
cases=[]
for f,source_body in forms.items():
 p=T/(f+".cpp");p.write_text(src[:lo]+source_body+src[hi:])
 for j,l,k in itertools.product((1,2,4),(1,2,4,8),(1,4,8)):
  cases.append((f,p,j,l,k))
rank=[]
for f,p,j,l,k in cases:
 key=f"{f}-j{j}-label{l}-loop{k}"
 options=["-O2","-frename-registers",f"-falign-jumps={j}",f"-falign-labels={l}",f"-falign-loops={k}"]
 obj=T/(key+".o")
 run=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*options,"-c",str(p),"-o",str(obj)],capture_output=True,text=True)
 if run.returncode:print("ERROR",key,run.stderr[-120:],flush=True);continue
 row=next((n for n in subprocess.check_output(["nm","-S",str(obj)],text=True).splitlines() if n.endswith("_Z9TranslateR11ASTNodeListIP7ASTStmtE")),"")
 size=int(row.split()[1],16) if row else -1
 if size==119:
  try:
   score=m.run(obj,R/"vuc/config/translate_stmt_list_probe.json")
   rank.append((score["exact_bytes"],size,key))
   print("ORIGINAL_SIZE",key,score["exact_bytes"],score["different_offsets"][:9],flush=True)
   if score["exact"]:print("NEW_EXACT",str(p),options,flush=True);raise SystemExit(0)
  except ValueError as e:print("REJECT",key,str(e)[:120],flush=True)
 else:rank.append((0,size,key))
print("BEST")
for row in sorted(rank,reverse=True)[:35]:print(row)

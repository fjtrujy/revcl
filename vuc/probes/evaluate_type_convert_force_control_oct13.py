#!/usr/bin/env python3
"""Explore original VUC TypeConvertForce's genuine boolean/control-flow shapes.

All options perform the same 32-byte operand copies, then accept an
already-compatible pair or a requested integer conversion from the two
original type-2 and type-3 representations. No arbitrary register tricks,
assembly, binary bytes, or synthetic source declarations.
"""
from pathlib import Path
import subprocess, importlib.util
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/typeconvertforce-control-oct13");T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location("m",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
source=(R/"vuc/probes/value_type_convert_force_oct13.cpp").read_text()
start=source.index("    if(first.type == desired")
body=source[start:source.rindex("}")]
split=body.index("    switch(left.type)")
left=body[split:body.index("    switch(right.type)",split)]
right=body[body.index("    switch(right.type)",split):body.rfind("    return true;")]
alternatives={
 "baseline":body,
 "separate-if":"""    if(first.type == desired) {
        if(second.type == desired) return true;
    }
    if(desired != ValueInteger) return false;
"""+left+right+"    return true;",
 "if-outer":"""    if(first.type != desired || second.type != desired) {
        if(desired != ValueInteger) return false;
"""+left+right+"""    }
    return true;""",
 "if-outer-reverse":"""    if(desired == ValueInteger) {
        if(first.type == desired && second.type == desired) return true;
"""+left+right+"""    } else {
        if(first.type != desired || second.type != desired) return false;
    }
    return true;""",
 "if-exit":"""    if(!(first.type == desired && second.type == desired)) {
        if(desired != ValueInteger) return false;
"""+left+right+"""    }
    return true;""",
 "switch-desired":"""    if(first.type == desired && second.type == desired) return true;
    switch(desired) {
    case ValueInteger:
"""+left+right+"""        break;
    default:
        return false;
    }
    return true;""",
 "if-skip":"""    if(first.type != desired || second.type != desired) {
        if(desired == ValueInteger) {
"""+left+right+"""        } else return false;
    }
    return true;""",
 "both-inequalities":"""    if(first.type == desired && second.type == desired) return true;
    if(desired == ValueInteger) {
"""+left+right+"""        return true;
    }
    return false;""",
 "or-separate":"""    if (first.type != desired || second.type != desired) {
        if (desired != ValueInteger) return false;
"""+left+right+"""        return true;
    }
    return true;""",
}
case_variants={
 "switch":(left,right),
 "switch-reordered":(left.replace("    case 2:","    case 2:").replace("    case 3:","    case 3:"),right),
 "switch-reverse":(left.replace("    case 2:\n        left.type=2;\n        break;\n    case 3:", "    case 3:").replace("        break;\n    default:", "        break;\n    case 2: left.type=2; break;\n    default:"),
                   right.replace("    case 2:\n        right.type=2;\n        break;\n    case 3:", "    case 3:").replace("        break;\n    default:", "        break;\n    case 2: right.type=2; break;\n    default:")),
 "if-predicates":("""    if (left.type == 2) { left.type=2; }
    else if(left.type == 3) {
      left.type=3; left.bits=(int)left.values[0];
    } else return false;
""", """    if (right.type == 2) { right.type=2; }
    else if(right.type == 3) {
      right.type=3; right.bits=(int)right.values[0];
    } else return false;
"""),
 "if-predicates-reverse":("""    if(left.type == 3) {
      left.type=3; left.bits=(int)left.values[0];
    } else if (left.type == 2) { left.type=2; }
    else return false;
""", """    if(right.type == 3) {
      right.type=3; right.bits=(int)right.values[0];
    } else if (right.type == 2) { right.type=2; }
    else return false;
"""),
}
flags={
"ren-j1":["-O2","-frename-registers","-falign-jumps=1"],
"ren-j4":["-O2","-frename-registers","-falign-jumps=4"],
"ren-j8":["-O2","-frename-registers","-falign-jumps=8"],
"plain-j1":["-O2","-falign-jumps=1"],
"ren":["-O2","-frename-registers"],
}
ranking=[]
for kind,pattern in alternatives.items():
 for variant,(ll,rr) in case_variants.items():
  if variant=="switch":text=pattern
  else:text=pattern.replace(left,ll).replace(right,rr)
  p=T/(kind+"-"+variant+".cpp")
  p.write_text(source[:start]+text+"\n}\n")
  for option,opts in flags.items():
   out=T/(p.stem+"-"+option+".o")
   cp=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*opts,"-c",str(p),"-o",str(out)],capture_output=True,text=True)
   if cp.returncode:
    print("ERROR",p.stem,option,cp.stderr[-105:],flush=True);continue
   output=subprocess.check_output(["nm","-S",str(out)],text=True)
   symbol=next((row for row in output.splitlines() if row.endswith("_ZN5Value16TypeConvertForceE6TypeIDRKS_S2_RS_S3_")),"")
   sz=int(symbol.split()[1],16) if symbol else -1
   if sz==257:
    try:
     d=m.run(out,R/"vuc/config/value_type_convert_force_oct13_probe.json")
     ranking.append((d["exact_bytes"],sz,p.stem,option,d["different_offsets"][:20]))
     if d["exact"]:
      print("NEW_EXACT",p,option,flush=True)
      raise SystemExit(0)
    except ValueError as e:ranking.append((0,sz,p.stem,option,str(e)[:105]))
   else:ranking.append((0,sz,p.stem,option,[]))
  print("checked",p.stem,flush=True)
print("BEST")
for row in sorted(ranking,reverse=True)[:33]:print(row)

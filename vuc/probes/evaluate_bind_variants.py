#!/usr/bin/env python3
"""Non-assembly source-shape sweep of VUC Bind AST list dispatch."""
import subprocess, importlib.util
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
TOOL=Path("/tmp/vuc-binutils215")
spec=importlib.util.spec_from_file_location("matcher",ROOT/"vuc/scripts/match_probe.py")
matcher=importlib.util.module_from_spec(spec);spec.loader.exec_module(matcher)
original=(ROOT/"vuc/probes/list_dispatch.cpp").read_text()
marker="    for (int i=0; i<list.length; ++i) {\n        T element = list.entries[i];\n        Bind(element, symbols);\n    }"
assert marker in original
bodies={
"base":"    for (int i=0; i<list.length; ++i) {\n        T element = list.entries[i];\n        Bind(element, symbols);\n    }",
"postincrement":"    int i=0;\n    while (i<list.length)\n        Bind(list.entries[i++], symbols);",
"for-inc-call":"    for (int i=0; i<list.length;)\n        Bind(list.entries[i++], symbols);",
"preload-pointer":"    for (int i=0; i<list.length; ++i) {\n        T *ptr = list.entries;\n        Bind(ptr[i], symbols);\n    }",
"preload-base":"    T *ptr = list.entries;\n    for (int i=0; i<list.length; ++i)\n        Bind(ptr[i], symbols);",
"increment-before":"    int i = 0;\n    while (i<list.length) {\n        T *ptr = list.entries;\n        ++i;\n        Bind(ptr[i-1], symbols);\n    }",
"preload-only":"    for (int i=0; i<list.length; ++i) {\n        T *base = list.entries;\n        T element = base[i];\n        Bind(element, symbols);\n    }",
"assign-after":"    for (int i=0; i<list.length; ++i) {\n        T temp = list.entries[i];\n        T element = temp;\n        Bind(element, symbols);\n    }",
"index-loop":"    int i = 0;\n    for (; i<list.length; ++i)\n        Bind(list.entries[i], symbols);",
"index-do":"    int i = 0;\n    if (i>=list.length) return;\n    do { Bind(list.entries[i], symbols); ++i; } while(i<list.length);",
"preload-int":"    for (int i=0; i<list.length; ++i) {\n        T element = *(list.entries+i);\n        Bind(element, symbols);\n    }",
"direct":"    for (int i=0; i<list.length; ++i)\n        Bind(list.entries[i], symbols);",
}
flags=[
("-O2-ren",["-O2","-frename-registers"]),
("-O2-j1",["-O2","-falign-jumps=1"]),
("-O2-ren-j1",["-O2","-frename-registers","-falign-jumps=1"]),
("-O2-ren-no-sched",["-O2","-frename-registers","-fno-schedule-insns2"]),
("-O2-ren-no-gcse",["-O2","-frename-registers","-fno-gcse"]),
]
out=[]
for name,body in bodies.items():
    path=TOOL/("bind-shape-"+name+".cpp")
    path.write_text(original.replace(marker,body))
    for opts_name,opts in flags:
        obj=TOOL/("bind-shape-"+name+opts_name+".o")
        proc=subprocess.run([str(TOOL/"g++-jan25-as215"),*opts,
                            "-I",str(ROOT/"vuc/probes"),
                            "-c",str(path),"-o",str(obj)],capture_output=True,text=True)
        if proc.returncode:
            out.append((0,0,name,opts_name,proc.stderr[-140:]));continue
        scores=[]
        for key in ("bind_expr","bind_stmt","bind_var","bind_decl"):
            try:
                result=matcher.run(obj,ROOT/("vuc/config/list_dispatch_"+key+"_probe.json"))
                scores.append(result["exact_bytes"])
            except Exception:
                scores.append(0)
        out.append((sum(scores),max(scores),name,opts_name,str(scores)))
out.sort(reverse=True)
for entry in out[:35]: print(*entry)

#!/usr/bin/env python3
"""VUC Instruction cleanup: genuine guard-arm topology and GCC 3.3 branch-cost.

Based on VCL prepExtInfo/doUnique evidence; it remains a hypothesis
for VUC's different compiler. Never force scratch pop registers.
"""
from pathlib import Path
import importlib.util,subprocess,itertools
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/instruction-guards-rebased-oct14")
T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location("match",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
src=(R/"vuc/probes/instruction_full_source_unit_oct13.cpp").read_text().replace(
    '#include "../src/MLGameString.h"','#include "'+str(R/"vuc/src/MLGameString.h")+'"')
needle="""        while (Instruction::instrData) {
            char *current = Instruction::instrData;
            char *next = *(char **)(current + 0x2004);
            ::operator delete(current);
            Instruction::instrData = next;
        }"""
assert needle in src
variants={
"original":needle,
"if-neg-do":"""        if (!Instruction::instrData) return;
        do {
            char *current = Instruction::instrData;
            char *next = *(char **)(current + 0x2004);
            ::operator delete(current);
            Instruction::instrData = next;
        } while (Instruction::instrData);""",
"if-pos-do":"""        if (Instruction::instrData) {
            do {
                char *current = Instruction::instrData;
                char *next = *(char **)(current + 0x2004);
                ::operator delete(current);
                Instruction::instrData = next;
            } while (Instruction::instrData);
        }""",
"while-branch-guard":"""        for (;;) {
            if (!Instruction::instrData) break;
            char *current = Instruction::instrData;
            char *next = *(char **)(current + 0x2004);
            ::operator delete(current);
            Instruction::instrData = next;
        }""",
"while-nested":"""        while (true) {
            char *current = Instruction::instrData;
            if (current) {
                char *next = *(char **)(current + 0x2004);
                ::operator delete(current);
                Instruction::instrData = next;
            } else break;
        }""",
"guard-current":"""        while (char *current = Instruction::instrData) {
            char *next = *(char **)(current + 0x2004);
            ::operator delete(current);
            Instruction::instrData = next;
        }""",
"while-after-next":"""        char *current = Instruction::instrData;
        while (current) {
            char *next = *(char **)(current + 0x2004);
            ::operator delete(current);
            Instruction::instrData = next;
            current = Instruction::instrData;
        }""",
"for-advance":"""        for (char *current = Instruction::instrData; current;
             current = Instruction::instrData) {
            char *next = *(char **)(current + 0x2004);
            ::operator delete(current);
            Instruction::instrData = next;
        }""",
"read-after":"""        while (Instruction::instrData) {
            char *current = Instruction::instrData;
            char *next = *(char **)(current + 0x2004);
            Instruction::instrData = next;
            ::operator delete(current);
        }""",
"conditional-continue":"""        for (;;) {
            if (!Instruction::instrData) return;
            char *current = Instruction::instrData;
            char *next = *(char **)(current + 0x2004);
            ::operator delete(current);
            Instruction::instrData = next;
            if (next) continue;
            return;
        }""",
"conditional-break":"""        char *current = Instruction::instrData;
        for (;;) {
            if (!current) break;
            char *next = *(char **)(current + 0x2004);
            ::operator delete(current);
            Instruction::instrData = next;
            current = next;
        }""",
}
base=["-O2","-frename-registers","-fuse-cxa-atexit","-ffunction-sections"]
flagsets={"base":[], "cost0":["-mbranch-cost=0"],"cost1":["-mbranch-cost=1"],"cost2":["-mbranch-cost=2"],
"cost3":["-mbranch-cost=3"],"cost4":["-mbranch-cost=4"],"cost5":["-mbranch-cost=5"],
"no-guess":["-fno-guess-branch-probability"],"no-thread":["-fno-thread-jumps"],
"no-ifconv":["-fno-if-conversion"],"no-ifconv2":["-fno-if-conversion2"],
"no-reorder":["-fno-reorder-blocks"],"no-sched2":["-fno-schedule-insns2"],
"no-regmove":["-fno-regmove"],"no-peephole2":["-fno-peephole2"],"cost0-no-guess":["-mbranch-cost=0","-fno-guess-branch-probability"]}
scored=[]
for name,body in variants.items():
 cpp=T/(name+".cpp");cpp.write_text(src.replace(needle,body))
 for snap,cxx in (("jan25","/tmp/vuc-binutils215/g++-jan25-as215"),("dec06","/tmp/vuc-binutils215/g++-dec06-as215")):
  for label,extras in flagsets.items():
   obj=T/(name+"-"+snap+"-"+label+".o")
   flags=base+extras
   build=subprocess.run([cxx,*flags,"-c",str(cpp),"-o",str(obj)],capture_output=True,text=True)
   if build.returncode:
    print("COMPILE",name,snap,label,build.stderr[-100:],flush=True);continue
   try:
    result=m.run(obj,R/"vuc/config/instruction_arena_cleanup_probe.json")
    scored.append((result["exact_bytes"],result["length"],name,snap,label,result["different_offsets"][:7]))
    if result["exact"]:
     print("NEW_EXACT",cpp,snap,label,flags,flush=True);raise SystemExit(0)
   except ValueError as e:
    scored.append((0,-1,name,snap,label,"REJECT "+str(e)[:95]))
 print("CHECKED",name,flush=True)
print("TOP")
for row in sorted(scored,reverse=True)[:35]:print(row,flush=True)

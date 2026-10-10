#!/usr/bin/env python3
"""Sweep original-ELF Get function across plausible GCC 3.3.3 configurations."""
from pathlib import Path
import importlib.util, subprocess
ROOT=Path(__file__).resolve().parents[2];TOOL=Path("/tmp/vuc-binutils215")
sp=importlib.util.spec_from_file_location("matcher",ROOT/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
raw=(ROOT/"vuc/probes/ml_string_table_get.cpp").read_text()
cfg=ROOT/"vuc/config/ml_string_table_get_probe.json"
source_variants={
"base":raw,
"ctor-body":raw.replace("MLGameStringTable() : count_(0), used_(0) {}", "MLGameStringTable() { count_ = 0; used_ = 0; }"),
"ctor-reverse":raw.replace("MLGameStringTable() : count_(0), used_(0) {}", "MLGameStringTable() { used_ = 0; count_ = 0; }"),
"ctor-init-reverse":raw.replace("MLGameStringTable() : count_(0), used_(0) {}", "MLGameStringTable() : used_(0), count_(0) {}"),
"field-reverse":raw.replace("unsigned count_;\n    unsigned used_;", "unsigned used_;\n    unsigned count_;"),
"field-reverse-body":raw.replace("unsigned count_;\n    unsigned used_;", "unsigned used_;\n    unsigned count_;").replace("MLGameStringTable() : count_(0), used_(0) {}", "MLGameStringTable() { count_ = 0; used_ = 0; }"),
}
opts={
"ren":["-O2","-frename-registers","-fuse-cxa-atexit"],
"ren-j1":["-O2","-frename-registers","-fuse-cxa-atexit","-falign-jumps=1"],
"ren-jl1":["-O2","-frename-registers","-fuse-cxa-atexit","-falign-labels=1"],
"ren-no-sched":["-O2","-frename-registers","-fuse-cxa-atexit","-fno-schedule-insns2"],
"ren-no-cse":["-O2","-frename-registers","-fuse-cxa-atexit","-fno-cse-follow-jumps"],
"ren-no-gcse":["-O2","-frename-registers","-fuse-cxa-atexit","-fno-gcse"],
"ren-no-peephole":["-O2","-frename-registers","-fuse-cxa-atexit","-fno-peephole2"],
"ren-no-reorder":["-O2","-frename-registers","-fuse-cxa-atexit","-fno-reorder-blocks"],
"ren-i686":["-O2","-frename-registers","-fuse-cxa-atexit","-march=i686"],
"ren-i386":["-O2","-frename-registers","-fuse-cxa-atexit","-march=i386"],
"plain":["-O2","-fuse-cxa-atexit"],
"plain-j1":["-O2","-fuse-cxa-atexit","-falign-jumps=1"],
"plain-no-sched":["-O2","-fuse-cxa-atexit","-fno-schedule-insns2"],
"ren-no-strict":["-O2","-frename-registers","-fuse-cxa-atexit","-fno-strict-aliasing"],
"ren-no-regmove":["-O2","-frename-registers","-fuse-cxa-atexit","-fno-regmove"]
}
scores=[]
for source_name,source in source_variants.items():
 cpp=TOOL/("table-get-"+source_name+".cpp");cpp.write_text(source)
 for flag_name,flags in opts.items():
  obj=TOOL/("table-get-"+source_name+"-"+flag_name+".o")
  cp=subprocess.run([str(TOOL/"g++-jan25-as215"),*flags,"-c",str(cpp),"-o",str(obj)],
                    capture_output=True,text=True)
  if cp.returncode:
   scores.append((0,source_name,flag_name,cp.stderr[-150:]));continue
  try:
   r=m.run(obj,cfg)
   scores.append((r["exact_bytes"],source_name,flag_name,str(r["length"])))
  except Exception as e:scores.append((0,source_name,flag_name,str(e)[:80]))
scores.sort(reverse=True)
for line in scores[:40]: print(*line)

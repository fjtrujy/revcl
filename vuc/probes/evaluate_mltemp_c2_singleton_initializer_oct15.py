#!/usr/bin/env python3
"""VUC MLTempString C2: real singleton data-constructor source variants.

Original C2 at 0x08071c20 (101 bytes) differs from reconstructed code at six
register-choice locations, notably the singleton's memset fill and size
arguments. Probe period-plausible MLTempStringData initialization C++03 source
without register asm, dummy predecessors, forged relocations or binary edits.
"""
from pathlib import Path
import importlib.util,itertools,json,subprocess,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location("m",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=R/"vuc/config/ml_temp_default_C2_probe.json"
header=(R/"vuc/src/MLTempStringData.h").read_text()
base=(R/"vuc/probes/ml_temp_constructor_unit_oct12.cpp").read_text()
assert '#include "../src/MLTempStringData.h"' in base
start=header.index("    MLTempStringData() {")
end=header.index("    char *FormatString(",start)
forms={
 "baseline":"""    MLTempStringData() { memset(bytes_, 'X', sizeof bytes_); bytes_[0x1fbf] = 0; used_ = 0; }""",
 "fill-int":"""    MLTempStringData() { const int fill = 0x58; memset(bytes_, fill, sizeof bytes_); bytes_[0x1fbf] = 0; used_ = 0; }""",
 "fill-char":"""    MLTempStringData() { const char fill = 'X'; memset(bytes_, fill, sizeof bytes_); bytes_[0x1fbf] = 0; used_ = 0; }""",
 "size-int":"""    MLTempStringData() { const int amount = sizeof bytes_; memset(bytes_, 'X', amount); bytes_[0x1fbf] = 0; used_ = 0; }""",
 "size-unsigned":"""    MLTempStringData() { const unsigned amount = sizeof bytes_; memset(bytes_, 'X', amount); bytes_[0x1fbf] = 0; used_ = 0; }""",
 "size-size_t":"""    MLTempStringData() { const std::size_t amount = sizeof bytes_; memset(bytes_, 'X', amount); bytes_[0x1fbf] = 0; used_ = 0; }""",
 "ptr-local":"""    MLTempStringData() { char *buffer = bytes_; memset(buffer, 'X', sizeof bytes_); bytes_[0x1fbf] = 0; used_ = 0; }""",
 "ptr-const":"""    MLTempStringData() { char *const buffer = bytes_; memset(buffer, 'X', sizeof bytes_); bytes_[0x1fbf] = 0; used_ = 0; }""",
 "ptr-first":"""    MLTempStringData() { char *buffer = &bytes_[0]; memset(buffer, 'X', sizeof bytes_); buffer[0x1fbf] = 0; used_ = 0; }""",
 "ptr-this":"""    MLTempStringData() { memset((void *)this, 'X', sizeof bytes_); bytes_[0x1fbf] = 0; used_ = 0; }""",
 "fill-hex":"""    MLTempStringData() { memset(bytes_, 0x58, 0x2fc0); bytes_[0x1fbf] = 0; used_ = 0; }""",
 "ptr-array":"""    MLTempStringData() { memset(&bytes_[0], 'X', sizeof bytes_); bytes_[0x1fbf] = 0; used_ = 0; }""",
 "flag-first":"""    MLTempStringData() { used_ = 0; memset(bytes_, 'X', sizeof bytes_); bytes_[0x1fbf] = 0; }""",
 "flag-between":"""    MLTempStringData() { memset(bytes_, 'X', sizeof bytes_); used_ = 0; bytes_[0x1fbf] = 0; }""",
 "idx-pointer":"""    MLTempStringData() { memset(bytes_, 'X', sizeof bytes_); char *end = &bytes_[0x1fbf]; *end = 0; used_ = 0; }""",
 "used-lvalue":"""    MLTempStringData() { memset(bytes_, 'X', sizeof bytes_); bytes_[0x1fbf] = 0; unsigned &current = used_; current = 0; }""",
 "ptr-ref":"""    MLTempStringData() { char *const &buffer = bytes_; memset(buffer, 'X', sizeof bytes_); bytes_[0x1fbf] = 0; used_ = 0; }""",
}
flags={
 "o2":["-O2"],
 "ren":["-O2","-frename-registers"],
 "no-sched":["-O2","-fno-schedule-insns2"],
 "no-regmove":["-O2","-fno-regmove"],
 "r-j1":["-O2","-frename-registers","-falign-jumps=1"],
}
T=Path("/tmp/vuc-binutils215/mltemp-c2-singleton-initializer-oct15");T.mkdir(parents=True,exist_ok=True)
rows=[];sizes=collections.Counter();best=None
for (form,ctor),(opt,opts),snap in itertools.product(forms.items(),flags.items(),("jan25","dec06")):
 name=form+"-"+opt+"-"+snap
 h=T/(name+".h");cpp=T/(name+".cpp");obj=T/(name+".o")
 h.write_text(header[:start]+ctor+"\n"+header[end:])
 cpp.write_text(base.replace('#include "../src/MLTempStringData.h"','#include "'+str(h)+'"'))
 cc=subprocess.run(['/tmp/vuc-binutils215/g++-'+snap+'-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if cc.returncode:rows.append(dict(name=name,error=cc.stderr[-150:]));continue
 try:res=m.run(obj,cfg)
 except Exception as e:rows.append(dict(name=name,error=str(e)));continue
 row=dict(name=name,length=res['length'],relocations=res['relocations'],overlap=res['exact_bytes'],prefix=next(iter(res['different_offsets']),res['length']),different_offsets=res['different_offsets'])
 rows.append(row);sizes[row['length']]+=1
 if row['length']==101 and row['relocations']==9 and (best is None or (row['overlap'],row['prefix'])>(best['overlap'],best['prefix'])):
  best=row;(T/"best.h").write_text(h.read_text());(T/"best.cpp").write_text(cpp.read_text());(T/"best.flags").write_text(' '.join(opts)+"\n");(T/"best.o").write_bytes(obj.read_bytes())
(T/"results.json").write_text(json.dumps(rows,indent=2)+"\n")
print(json.dumps(dict(attempted=len(rows),sizes=dict(sizes),best_original_length=best,top=sorted((r for r in rows if "overlap" in r),key=lambda x:(x["overlap"],x["prefix"]),reverse=True)[:12]),indent=2))

#!/usr/bin/env python3
"""Natural C++03 register lifetime controls for VUC MLTempString::Set.

Target original 117 bytes; candidate 111/117 with 8 authentic relocation
sites. Probe real variadic/field/data-owner source spellings; no asm, dummy
predecessors, machine-code patching or fabricated relocation addresses.
"""
from pathlib import Path
import importlib.util,json,subprocess,itertools,collections
R=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location("m",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
cfg=json.loads((R/"vuc/config/ml_temp_set_probe.json").read_text())
raw,elf=m.decomp.load();section=next(x for x in elf["sections"] if x["name"]==".text")
addr=int(cfg["address"],0)
ref=raw[section["offset"]+addr-section["address"]:section["offset"]+addr-section["address"]+cfg["size"]]
base=(R/cfg["source"]).read_text().replace('"../src/MLTempStringData.h"','"'+str(R/"vuc/src/MLTempStringData.h")+'"')
lo=base.index("    va_list arguments;")
hi=base.index("    return this;",lo)+len("    return this;")
signature="MLTempString *MLTempString::Set(const char *format, ...)"
variants={
 "baseline":base[lo:hi],
 "fmt-local":"""    const char *fmt = format;
    va_list arguments;
    va_start(arguments, format);
    text = MLTempStringData::Get()->VFormatString(fmt, (char *)arguments);
    va_end(arguments);
    return this;""",
 "fmt-reference":"""    const char *const &fmt = format;
    va_list arguments;
    va_start(arguments, format);
    text = MLTempStringData::Get()->VFormatString(fmt, (char *)arguments);
    va_end(arguments);
    return this;""",
 "fmt-const-pointer":"""    const char *const fmt = format;
    va_list arguments;
    va_start(arguments, format);
    text = MLTempStringData::Get()->VFormatString(fmt, (char *)arguments);
    va_end(arguments);
    return this;""",
 "data-local":"""    va_list arguments;
    va_start(arguments, format);
    MLTempStringData *owner = MLTempStringData::Get();
    text = owner->VFormatString(format, (char *)arguments);
    va_end(arguments);
    return this;""",
 "data-const":"""    va_list arguments;
    va_start(arguments, format);
    MLTempStringData *const owner = MLTempStringData::Get();
    text = owner->VFormatString(format, (char *)arguments);
    va_end(arguments);
    return this;""",
 "this-owner":"""    va_list arguments;
    va_start(arguments, format);
    MLTempString *self = this;
    self->text = MLTempStringData::Get()->VFormatString(format, (char *)arguments);
    va_end(arguments);
    return self;""",
 "this-const-owner":"""    va_list arguments;
    va_start(arguments, format);
    MLTempString *const self = this;
    self->text = MLTempStringData::Get()->VFormatString(format, (char *)arguments);
    va_end(arguments);
    return self;""",
 "result-local":"""    va_list arguments;
    va_start(arguments, format);
    const char *formatted = MLTempStringData::Get()->VFormatString(format, (char *)arguments);
    text = formatted;
    va_end(arguments);
    return this;""",
 "result-reference":"""    va_list arguments;
    va_start(arguments, format);
    const char *const &formatted = MLTempStringData::Get()->VFormatString(format, (char *)arguments);
    text = formatted;
    va_end(arguments);
    return this;""",
 "return-local":"""    va_list arguments;
    va_start(arguments, format);
    MLTempString *result = this;
    result->text = MLTempStringData::Get()->VFormatString(format, (char *)arguments);
    va_end(arguments);
    return result;""",
 "va-end-before-store":"""    va_list arguments;
    va_start(arguments, format);
    const char *formatted = MLTempStringData::Get()->VFormatString(format, (char *)arguments);
    va_end(arguments);
    text = formatted;
    return this;""",
}
flags={
 "o2":["-O2"],
 "rename":["-O2","-frename-registers"],
 "j4":["-O2","-falign-jumps=4"],
 "j8":["-O2","-falign-jumps=8"],
 "no-regmove":["-O2","-fno-regmove"],
 "no-sched2":["-O2","-fno-schedule-insns2"],
 "no-cse-follow":["-O2","-fno-cse-follow-jumps"],
}
T=Path("/tmp/vuc-binutils215/mltemp-set-real-lifetimes-next-oct14");T.mkdir(parents=True,exist_ok=True)
scores=[];sizes=collections.Counter();best=None
for (name,body),(label,opts),snapshot in itertools.product(variants.items(),flags.items(),("jan25","dec06")):
 tag=f"{name}-{label}-{snapshot}"
 code=base[:lo]+body+base[hi:]
 src=T/(tag+".cpp");obj=T/(tag+".o");src.write_text(code)
 cp=subprocess.run(["/tmp/vuc-binutils215/g++-"+snapshot+"-as215",*opts,"-c",str(src),"-o",str(obj)],capture_output=True,text=True)
 if cp.returncode:
  scores.append(dict(tag=tag,error=cp.stderr[-150:]));continue
 try:candidate,rel=m.prepare_named_function(obj.read_bytes(),cfg["function"],cfg["relocations"],addr,cfg.get("relocation_objects"),cfg.get("relocation_literals"))
 except Exception as e:
  # Use high-level checker below if this helper does not support object relocation details.
  try:check=m.run(obj,R/"vuc/config/ml_temp_set_probe.json")
  except Exception as e2:
   scores.append(dict(tag=tag,error=str(e2)));continue
  row=dict(tag=tag,length=check["length"],relocations=check["relocations"],overlap=check["exact_bytes"],prefix=-1)
 else:
  overlap=sum(a==b for a,b in zip(candidate,ref))
  prefix=next((i for i,(a,b) in enumerate(zip(candidate,ref)) if a!=b),min(len(candidate),len(ref)))
  row=dict(tag=tag,length=len(candidate),relocations=rel,overlap=overlap,prefix=prefix)
 scores.append(row);sizes[row["length"]]+=1
 if row["length"]==117 and row["relocations"]==8 and (best is None or (row["overlap"],row["prefix"])>(best["overlap"],best["prefix"])):
  best=row;(T/"best.cpp").write_text(code);(T/"best.flags").write_text(" ".join(opts)+"\n");(T/"best.o").write_bytes(obj.read_bytes())
(T/"results.json").write_text(json.dumps(scores,indent=2)+"\n")
print(json.dumps(dict(attempted=len(scores),sizes=dict(sizes),best_original_length=best,top=sorted((x for x in scores if "overlap" in x),key=lambda r:(r["overlap"],r["prefix"]),reverse=True)[:12]),indent=2))

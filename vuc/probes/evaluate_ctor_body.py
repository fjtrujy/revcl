#!/usr/bin/env python3
"""Explore equivalent varargs constructor source sequences for both ABI entrypoints."""
from pathlib import Path
import importlib.util,subprocess
ROOT=Path(__file__).resolve().parents[2]
TOOL=Path("/tmp/vuc-binutils215")
sp=importlib.util.spec_from_file_location("match",ROOT/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
raw=(ROOT/"vuc/probes/ml_temp_format_inline.cpp").read_text()
start=raw.index("MLTempString::MLTempString(const char *format, ...)")
start_body=raw.index("{",start)
end=raw.index("\n}",start_body)+2
bodies={
"original":raw[start_body:end],
"pointer-first":'''{
    MLTempStringData *buffer = MLTempStringData::Get();
    va_list arguments;
    va_start(arguments, format);
    text = buffer->VFormatString(format, (char *)arguments);
    va_end(arguments);
}''',
"assign-after":'''{
    va_list arguments;
    va_start(arguments, format);
    char *formatted = MLTempStringData::Get()->VFormatString(format, (char *)arguments);
    va_end(arguments);
    text = formatted;
}''',
"assign-before-end":'''{
    va_list arguments;
    va_start(arguments, format);
    char *formatted = MLTempStringData::Get()->VFormatString(format, (char *)arguments);
    text = formatted;
    va_end(arguments);
}''',
"no-end":'''{
    va_list arguments;
    va_start(arguments, format);
    text = MLTempStringData::Get()->VFormatString(format, (char *)arguments);
}''',
"ptr-before-args":'''{
    MLTempStringData *buffer = MLTempStringData::Get();
    va_list arguments;
    va_start(arguments, format);
    char *formatted = buffer->VFormatString(format, (char *)arguments);
    text = formatted;
    va_end(arguments);
}''',
"ptr-after-args":'''{
    va_list arguments;
    va_start(arguments, format);
    MLTempStringData *buffer = MLTempStringData::Get();
    text = buffer->VFormatString(format, (char *)arguments);
    va_end(arguments);
}''',
"arg-cast":'''{
    va_list arguments;
    va_start(arguments, format);
    char *pointer = (char *)arguments;
    text = MLTempStringData::Get()->VFormatString(format, pointer);
    va_end(arguments);
}''',
"const-format":'''{
    va_list arguments;
    va_start(arguments, format);
    const char *fmt = format;
    text = MLTempStringData::Get()->VFormatString(fmt, (char *)arguments);
    va_end(arguments);
}''',
"this-field":'''{
    va_list arguments;
    va_start(arguments, format);
    this->text = MLTempStringData::Get()->VFormatString(format, (char *)arguments);
    va_end(arguments);
}''',
"direct-stack":'''{
    text = MLTempStringData::Get()->VFormatString(format, (char *)(&format + 1));
}''',
"pointer-form":'''{
    va_list arguments;
    va_start(arguments, format);
    MLTempStringData *buffer = MLTempStringData::Get();
    char *value = buffer->VFormatString(format, (char *)arguments);
    va_end(arguments);
    this->text = value;
}''',
}
opts={
"ren-j1":["-O2","-frename-registers","-falign-jumps=1"],
"ren-jl1":["-O2","-frename-registers","-falign-jumps=1","-falign-labels=1"],
"O2-j1":["-O2","-falign-jumps=1"],
"ren-j1-no-sched":["-O2","-frename-registers","-falign-jumps=1","-fno-schedule-insns2"],
}
results=[]
for name,body in bodies.items():
 source=TOOL/("ctor-body-"+name+".cpp")
 source.write_text(raw[:start_body]+body+raw[end:])
 for flagkey,flags in opts.items():
    obj=TOOL/("ctor-body-"+name+"-"+flagkey+".o")
    proc=subprocess.run([str(TOOL/"g++-jan25-as215"),*flags,"-I",str(ROOT/"vuc/src"),
                         "-c",str(source),"-o",str(obj)],capture_output=True,text=True)
    if proc.returncode:
       results.append((0,0,name,flagkey,proc.stderr[-130:]));continue
    a=[]
    for filename in ("ml_temp_format_c2_probe.json","ml_temp_format_c1_probe.json"):
       try:
           r=m.run(obj,ROOT/"vuc/config"/filename)
           a.append((r["exact_bytes"],r["length"]))
       except Exception as e:a.append((0,str(e)[:60]))
    results.append((a[0][0],a[1][0],name,flagkey,str(a)))
results.sort(reverse=True)
for result in results[:35]:
 print(*result)

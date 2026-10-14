#!/usr/bin/env python3
"""Test actual MLTempString constructors/format owner before 117-byte Set.

The six differing bytes in Set are register assignments for live arguments
and initializer constants, not semantic call/relocation mistakes. The
candidate's genuine preceding constructors may influence GCC 3.3 pass state.
"""
import importlib.util
import itertools
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TMP = Path("/tmp/vuc-binutils215/mltemp-real-prefix-oct14")
TMP.mkdir(parents=True, exist_ok=True)
sp = importlib.util.spec_from_file_location("match", ROOT / "vuc/scripts/match_probe.py")
match = importlib.util.module_from_spec(sp)
sp.loader.exec_module(match)
header = """#include <cstdarg>
#include "MLTempStringData.h"
struct MLTempString {
    const char *text;
    MLTempString();
    MLTempString(const char *format, ...);
    MLTempString *Set(const char *format, ...);
};
inline MLTempStringData *MLTempStringData::Get()
{
    static MLTempStringData tsd;
    return &tsd;
}
"""
parts = {
 "formatted": """MLTempString::MLTempString(const char *format, ...)
{
    va_list args; va_start(args, format);
    text = MLTempStringData::Get()->VFormatString(format, (char *)args);
    va_end(args);
}
""",
 "default": """MLTempString::MLTempString()
{
    text = MLTempStringData::Get()->FormatString("");
}
""",
 "format": """char *MLTempStringData::FormatString(const char *format, ...)
{
    va_list args; va_start(args, format);
    char *result = VFormatString(format, (char *)args);
    va_end(args); return result;
}
""",
 "set": """MLTempString *MLTempString::Set(const char *format, ...)
{
    va_list args; va_start(args, format);
    text = MLTempStringData::Get()->VFormatString(format, (char *)args);
    va_end(args);
    return this;
}
""",
}
variants = [("set",)]
for k in (1, 2, 3):
    for predecessor in itertools.permutations(("formatted", "default", "format"), k):
        variants.append((*predecessor, "set"))
results = []
for v in variants:
    name = "-".join(v)
    cpp = TMP / (name + ".cpp")
    cpp.write_text((header + "\n".join(parts[n] for n in v)).replace(
        '#include "MLTempStringData.h"',
        '#include "' + str(ROOT / "vuc/src/MLTempStringData.h") + '"'))
    for flag, flags in {"plain":["-O2"], "rename":["-O2", "-frename-registers"]}.items():
        obj = TMP / (name + "-" + flag + ".o")
        res = subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",
                              *flags, "-c", str(cpp), "-o", str(obj)],
                             text=True, capture_output=True)
        if res.returncode:
            print("COMPILE_ERROR", name, flag, res.stderr[-130:], flush=True)
            continue
        nm = subprocess.check_output(["nm", "-S", str(obj)], text=True)
        symbol = next((x for x in nm.splitlines()
                       if x.split()[-1:] == ["_ZN12MLTempString3SetEPKcz"]), "")
        length = int(symbol.split()[1], 16) if symbol else -1
        if length != 117:
            results.append((0, length, name, flag, "length"))
            continue
        try:
            score = match.run(obj, ROOT / "vuc/config/ml_temp_set_probe.json")
            results.append((score["exact_bytes"], length, name, flag,
                            score["different_offsets"]))
            if score["exact"]:
                print("NEW_EXACT", name, flag, cpp, flush=True)
                raise SystemExit(0)
        except ValueError as err:
            results.append((0, length, name, flag, str(err)[:95]))
    print("CHECKED", name, flush=True)
for row in sorted(results, reverse=True)[:24]:
    print("SCORE", row)

#!/usr/bin/env python3
"""Historical assert-macro and Reserve() source ownership for VFormatString.

The original ELF has assert evidence at MLGameString.cpp:145 and :200.
Use those authenticated source line/file locations through #line and
ordinary C++03 assert to see whether GCC's automatic __PRETTY_FUNCTION__
constant placement reproduces original .rodata ownership. Never fabricate
a target address or map compiler padding to unrelated zero bytes.
"""
import json
from pathlib import Path
import subprocess

R = Path(__file__).resolve().parents[1]
O = Path("/tmp/vuc-binutils215/vformat-assert-owner-oct16")
O.mkdir(parents=True, exist_ok=True)
header = (R / "src/MLTempStringData.h").read_text()
header = header.replace("    char *VFormatString(",
                        "    unsigned Max() const { return 0x1fbf; }\n"
                        "    char *Reserve(int length);\n"
                        "    char *VFormatString(")
function_body = '''
char *MLTempStringData::VFormatString(const char *format, char *arguments)
{
    int strLen = strlen(format);
    char *start = Reserve(strLen + 1);
    int numWritten = vsprintf(start, format, (va_list)arguments);
    if (numWritten < 0) {
        used_ = 0;
        start = bytes_;
        numWritten = vsprintf(start, format, (va_list)arguments);
#line 200 "../megalib/MLGameString.cpp"
        assert(numWritten >= 0);
    }
    used_ += numWritten + 1;
    return start;
}
'''
reserve = '''
inline char *MLTempStringData::Reserve(int strLen)
{
    if (Max() - (int)used_ < strLen) {
        used_ = 0;
#line 145 "../megalib/MLGameString.cpp"
        assert(Max() > strLen);
    }
    return bytes_ + used_;
}
'''
forms = {
    "inline-reserve": reserve + function_body,
    "direct-assert": '''
char *MLTempStringData::VFormatString(const char *format, char *arguments)
{
    int strLen = strlen(format);
    if (Max() - (int)used_ < strLen + 1) {
        used_ = 0;
#line 145 "../megalib/MLGameString.cpp"
        assert(Max() > strLen);
    }
    char *start = bytes_ + used_;
    int numWritten = vsprintf(start, format, (va_list)arguments);
    if (numWritten < 0) {
        used_ = 0;
        start = bytes_;
        numWritten = vsprintf(start, format, (va_list)arguments);
#line 200 "../megalib/MLGameString.cpp"
        assert(numWritten >= 0);
    }
    used_ += numWritten + 1;
    return start;
}
''',
}
prefix = "#include <cstring>\n#include <cstdio>\n#include <cassert>\n#include <cstdarg>\n"
flags_list = {
    "O2": ["-O2"],
    "rename": ["-O2", "-frename-registers"],
    "noreorder": ["-O2", "-frename-registers", "-fno-reorder-blocks"],
    "j1": ["-O2", "-frename-registers", "-falign-jumps=1"],
}
rows = []
for name, body in forms.items():
    cpp = O / (name + ".cpp")
    cpp.write_text(prefix + header + body)
    for setting, flags in flags_list.items():
        obj = O / (name + "-" + setting + ".o")
        run = subprocess.run(
            ["/tmp/vuc-binutils215/g++-jan25-as215", *flags,
             "-c", str(cpp), "-o", str(obj)],
            capture_output=True, text=True)
        row = {"source": name, "setting": setting}
        if run.returncode:
            row["error"] = run.stderr[-200:]
        else:
            nm = subprocess.run(["nm", "-S", "--defined-only", str(obj)],
                                capture_output=True, text=True)
            funcs = [x.split() for x in nm.stdout.splitlines()
                     if x.endswith(" _ZN16MLTempStringData13VFormatStringEPKcPc")]
            row["length"] = int(funcs[0][1], 16) if funcs else None
            dump = subprocess.run(["objdump", "-s", "-j", ".rodata", str(obj)],
                                  capture_output=True, text=True)
            row["has_original_line"] = b"MLGameString.cpp" in bytes.fromhex(
                "".join(p for line in dump.stdout.splitlines()
                        if line.startswith(" ") and len(line.split()) >= 5
                        for p in line.split()[1:5]
                        if len(p) == 8 and all(c in "0123456789abcdef" for c in p))
            ) if dump.returncode == 0 else False
        rows.append(row)
        print(json.dumps(row, sort_keys=True), flush=True)
(O / "results.json").write_text(json.dumps(rows, indent=2) + "\n")

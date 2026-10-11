#!/usr/bin/env python3
"""Search natural diagnostics stream expressions, preserving original behavior."""
import importlib.util
from pathlib import Path
import subprocess

R = Path(__file__).resolve().parents[2]
T = Path("/tmp/vuc-binutils215")
spec = importlib.util.spec_from_file_location("vucmatch", R / "vuc/scripts/match_probe.py")
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)
source = (R / "vuc/probes/script_error.cpp").read_text()
body = '''    std::cout << currFileName;
    std::cout << "(" << script_line << "):"
              << message << std::endl;'''
assert body in source
variants = {
    "baseline": body,
    "begin-local": '''    std::cout << currFileName;
    const char *opening = "(";
    std::cout << opening << script_line << "):" << message << std::endl;''',
    "ending-local": '''    std::cout << currFileName;
    const char *closing = "):";
    std::cout << "(" << script_line << closing << message << std::endl;''',
    "both-local": '''    std::cout << currFileName;
    const char *opening = "(";
    const char *closing = "):";
    std::cout << opening << script_line << closing << message << std::endl;''',
    "endl-local": '''    std::cout << currFileName;
    std::ostream& (*line_end)(std::ostream&) = std::endl;
    std::cout << "(" << script_line << "):" << message << line_end;''',
    "stream-local": '''    std::ostream &out = std::cout;
    out << currFileName;
    out << "(" << script_line << "):" << message << std::endl;''',
    "paren-output-ref": '''    std::cout << currFileName;
    std::ostream &out = std::cout << "(";
    out << script_line << "):" << message << std::endl;''',
    "paren-output-ptr": '''    std::cout << currFileName;
    std::ostream *out = &(std::cout << "(");
    *out << script_line << "):" << message << std::endl;''',
    "explicit-operator": '''    std::operator<<(std::cout,currFileName);
    std::cout << "(" << script_line << "):" << message << std::endl;''',
    "explicit-opening": '''    std::cout << currFileName;
    std::operator<<(std::cout,"(") << script_line << "):" << message << std::endl;''',
    "explicit-ending": '''    std::cout << currFileName;
    std::cout << "(" << script_line;
    std::cout << "):" << message << std::endl;''',
    "separate-ending": '''    std::cout << currFileName;
    std::cout << "(" << script_line << "):" << message;
    std::cout << std::endl;''',
    "full-separate": '''    std::cout << currFileName;
    std::cout << "(";
    std::cout << script_line;
    std::cout << "):";
    std::cout << message;
    std::cout << std::endl;''',
    "comma": '''    (std::cout << currFileName, std::cout << "(")
        << script_line << "):" << message << std::endl;''',
    "endl-explicit": '''    std::cout << currFileName;
    std::cout << "(" << script_line << "):" << message
              << (std::ostream& (*)(std::ostream&))std::endl;''',
    "message-local": '''    const char *text=message;
    std::cout << currFileName;
    std::cout << "(" << script_line << "):" << text << std::endl;''',
}
variants = {**variants,
    "curr-nonconst": None,
    "curr-volatile": None,
    "line-unsigned": None,
    "error-volatile": None}
flags = {
    "ren-j1": ["-O2", "-frename-registers", "-falign-jumps=1"],
    "plain-j1": ["-O2", "-falign-jumps=1"],
    "ren-j1-no-gcse": ["-O2", "-frename-registers", "-falign-jumps=1", "-fno-gcse"],
}
results = []
for name, replacement in variants.items():
    content = source.replace(body, replacement) if replacement else source
    if name == "curr-nonconst":
        content = content.replace("extern const char *currFileName;", "extern char *currFileName;")
    elif name == "curr-volatile":
        content = content.replace("extern const char *currFileName;", "extern const char * volatile currFileName;")
    elif name == "line-unsigned":
        content = content.replace("extern int script_line;", "extern unsigned script_line;")
    elif name == "error-volatile":
        content = content.replace("extern bool error;", "extern volatile bool error;")
    path = T / ("script-error-natural-" + name + ".cpp")
    path.write_text(content)
    for flname, opts in flags.items():
        obj = T / ("script-error-natural-" + name + "-" + flname + ".o")
        run = subprocess.run([str(T / "g++-jan25-as215"), *opts,
                              "-c", str(path), "-o", str(obj)],
                             capture_output=True, text=True)
        if run.returncode:
            results.append((0, name, flname, run.stderr[-95:]))
            continue
        try:
            score = m.run(obj, R / "vuc/config/script_error_probe.json")
            results.append((score["exact_bytes"], name, flname,
                            str(score["length"]), score["different_offsets"]))
            if score["exact"]:
                print("NEW EXACT:", name, flname, flush=True)
        except ValueError as err:
            results.append((0, name, flname, str(err)[:100]))
for row in sorted(results, reverse=True)[:30]:
    print(*row)

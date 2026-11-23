#!/usr/bin/env python3
"""Rebuild TryOpenFile's natural C++03 ifstream lifetime in different source forms.

The selected source is an original-length, 17-relocation authenticated
650/754 instruction partial. Its first divergence is in the inlined stream
constructor / EH cleanup. Compare genuine object construction, return-state
and inline-helper contracts. Accept only original-length, exactly mapped
historical outputs; no artificial compiler padding or manually inserted ASM.
"""
from pathlib import Path
import importlib.util,json,subprocess
V=Path(__file__).resolve().parents[1]
O=Path("/tmp/vuc-try-open-source-20261123");O.mkdir(exist_ok=True)
orig=(V/"probes/TryOpenFileSuccessFirstOct15t.cpp").read_text()
preamble='#include <fstream>\n'
body0="""bool TryOpenFile(const char *path)
{
    std::ifstream input(path);
    if (!input.fail())
        return true;
    return false;
}
"""
assert body0 in orig
cases={
 "baseline":("",body0),
 "single-return":("",body0.replace("    if (!input.fail())\n        return true;\n    return false;", "    return !input.fail();")),
 "fail-first":("",body0.replace("    if (!input.fail())\n        return true;\n    return false;", "    if (input.fail()) return false;\n    return true;")),
 "conditional":("",body0.replace("    if (!input.fail())\n        return true;\n    return false;", "    return input.fail() ? false : true;")),
 "state-bool":("",body0.replace("    if (!input.fail())\n        return true;\n    return false;", "    return input ? true : false;")),
 "local-flag":("",body0.replace("    if (!input.fail())\n        return true;\n    return false;", "    const bool success = !input.fail();\n    return success;")),
 "nonconst-local":("",body0.replace("    if (!input.fail())\n        return true;\n    return false;", "    bool success = !input.fail();\n    if (success) return true;\n    return false;")),
 "open-in":("",body0.replace("std::ifstream input(path);","std::ifstream input(path,std::ios::in);")),
 "open-in-explicit":("",body0.replace("std::ifstream input(path);","std::ifstream input(path,std::ios_base::in);")),
 "stream-generic-type":("",body0.replace("std::ifstream input(path);","std::basic_ifstream<char> input(path);")),
 "stream-explicit-traits":("",body0.replace("std::ifstream input(path);","std::basic_ifstream<char, std::char_traits<char> > input(path);")),
 "local-path":("",body0.replace("std::ifstream input(path);","const char *const name = path;\n    std::ifstream input(name);")),
 "local-path-ref":("",body0.replace("std::ifstream input(path);","const char *const &name = path;\n    std::ifstream input(name);")),
 "local-ifstream-pointer":("",body0.replace("    if (!input.fail())","    std::ifstream *stream = &input;\n    if (!stream->fail())")),
 "local-ifstream-ref":("",body0.replace("    if (!input.fail())","    std::ifstream &stream = input;\n    if (!stream.fail())")),
 "input-empty-open":("",body0.replace("std::ifstream input(path);","std::ifstream input;\n    input.open(path);")),
 "empty-open-explicit":("",body0.replace("std::ifstream input(path);","std::ifstream input;\n    input.open(path,std::ios::in);")),
 "helper-test":(
  "static inline bool testFile(std::ifstream &input) {return !input.fail();}\n",
  body0.replace("    if (!input.fail())","    if (testFile(input))")),
 "helper-test-const":(
  "static inline bool testFile(const std::ifstream &input) {return !input.fail();}\n",
  body0.replace("    if (!input.fail())","    if (testFile(input))")),
 "helper-test-template":(
  "template<class T> static inline bool testFile(const T &input) {return !input.fail();}\n",
  body0.replace("    if (!input.fail())","    if (testFile(input))")),
 "helper-all":(
  "static inline bool tryOpenImpl(const char *name) {\n"
  "    std::ifstream input(name);\n"
  "    if (!input.fail()) return true;\n"
  "    return false;\n"
  "}\n",
  "bool TryOpenFile(const char *path) {return tryOpenImpl(path);}\n"),
 "helper-all-always":(
  "static inline __attribute__((always_inline)) bool tryOpenImpl(const char *name) {\n"
  "    std::ifstream input(name);\n"
  "    if (!input.fail()) return true;\n"
  "    return false;\n"
  "}\n",
  "bool TryOpenFile(const char *path) {return tryOpenImpl(path);}\n"),
 "helper-arg-reversed":(
  "static inline bool readStream(const char *path, std::ifstream &input) {\n"
  "    input.open(path); return !input.fail();\n"
  "}\n",
  "bool TryOpenFile(const char *path) {\n    std::ifstream input;\n    return readStream(path,input);\n}\n"),
}
spec=importlib.util.spec_from_file_location("m",V/"scripts/match_probe.py")
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
cfg=V/"config/try_open_file_partial_oct15t.json"
rows=[]
for label,(helper,body) in cases.items():
  src=O/(label+".cpp");src.write_text(preamble+helper+body)
  for compiler in ("jan25","dec06"):
    obj=O/(label+"-"+compiler+".o")
    cmd=["/tmp/vuc-binutils215/g++-"+compiler+"-as215",
      "-O2","-frename-registers","-falign-jumps=8","-c",str(src),"-o",str(obj)]
    p=subprocess.run(cmd,capture_output=True,text=True)
    row={"case":label,"compiler":compiler}
    if p.returncode:row["compilation_error"]=p.stderr[-300:]
    else:
      try:
        s=m.run(obj,cfg)
        row.update(length=s["length"],matches=s["exact_bytes"],exact=s["exact"],
                   diffs=s["different_offsets"][:70],relocations=s["relocations"])
      except ValueError as e:row["unverified"]=str(e)
    rows.append(row);print(json.dumps(row,sort_keys=True),flush=True)
(O/"results.json").write_text(json.dumps(rows,indent=2)+"\n")

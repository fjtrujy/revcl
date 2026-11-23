#!/usr/bin/env python3
"""VUC AsmOutput_WritePrologue: reverse-parameter inline helper experiment.

Original selects EAX for literal and EDX for ofstream before either stack
argument store, while historical GCC current body stores literal before it
loads ofstream into EAX. Test genuine reversed inline helper interfaces and
typed ostream casts that could change operand live ranges at the call.
No inline assembly or dummy register dependencies.
"""
from pathlib import Path
import importlib.util, json, subprocess
V=Path(__file__).resolve().parents[1]
O=Path("/tmp/vuc-prologue-reverse-arg-20261123");O.mkdir(exist_ok=True)
original=(V/"probes/asm_output_hypothesis.cpp").read_text()
prefix, body=original.split("void AsmOutput_WritePrologue(std::ofstream &out)",1)
start=body.index("    out << ")
stop=body.index(";\n",start)
expr=body[start+len("    out << "):stop].strip()
func_header="void AsmOutput_WritePrologue(std::ofstream &out)\n{\n    "
func_tail="\n}\n"
cases={
 "baseline":("", "out << LITERAL"),
 "inline-reverse-ofstream":(
   "static inline void emit(const char *text, std::ofstream &out) { out << text; }\n",
   "emit(LITERAL,out)"),
 "inline-reverse-ostream":(
   "static inline void emit(const char *text, std::ostream &out) { out << text; }\n",
   "emit(LITERAL,out)"),
 "inline-reverse-stream-ref":(
   "static inline std::ostream &emit(const char *text, std::ostream &out) { return out << text; }\n",
   "emit(LITERAL,out)"),
 "inline-reverse-stream-ptr":(
   "static inline void emit(const char *text, std::ostream *out) { *out << text; }\n",
   "emit(LITERAL,&out)"),
 "inline-reverse-ofstream-ptr":(
   "static inline void emit(const char *text, std::ofstream *out) { *out << text; }\n",
   "emit(LITERAL,&out)"),
 "inline-reverse-ofstream-template":(
   "template<typename T> static inline void emit(const char *text, T &out) { out << text; }\n",
   "emit(LITERAL,out)"),
 "inline-reverse-output-template":(
   "template<typename T> static inline void emit(const T &text, std::ofstream &out) { out << text; }\n",
   "emit(LITERAL,out)"),
 "inline-reverse-both-template":(
   "template<typename S, typename T> static inline void emit(const T &text, S &out) { out << text; }\n",
   "emit(LITERAL,out)"),
 "inline-reverse-callee-template":(
   "template<typename T> static inline void emit(T text, std::ostream &out) { out << text; }\n",
   "emit(LITERAL,out)"),
 "inline-reverse-std-call":(
   "static inline void emit(const char *text, std::ostream &out) { std::operator<<(out,text); }\n",
   "emit(LITERAL,out)"),
 "inline-reverse-std-ofstream-call":(
   "static inline void emit(const char *text, std::ofstream &out) { std::operator<<(out,text); }\n",
   "emit(LITERAL,out)"),
 "class-static-reverse":(
   "struct Printer { static inline void emit(const char* text, std::ostream& out) { out << text; }};\n",
   "Printer::emit(LITERAL,out)"),
 "class-reverse-template":(
   "template<typename T> struct Printer { static inline void emit(const char* text,T& out){out << text;}};\n",
   "Printer<std::ofstream>::emit(LITERAL,out)"),
 "pointer-conditional":(
   "static inline std::ostream& nonnull(std::ostream* out){return *out;}\n",
   "nonnull(&out) << LITERAL"),
 "reversed-lambda-equivalent":(
   "static inline std::ostream &outstream(const char *text, std::ostream &out) { (void)text; return out; }\n",
   "outstream(LITERAL,out) << LITERAL"),
}
sp=importlib.util.spec_from_file_location("m",V/"scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=V/"config/asm_output_jan25_probe.json"
rows=[]
for name,(helper,stmt) in cases.items():
  path=O/(name+".cpp")
  path.write_text(prefix+helper+"\n"+func_header+stmt.replace("LITERAL",expr)+";"+func_tail)
  for snap in ("jan25","dec06"):
    out=O/(name+"-"+snap+".o")
    cp=subprocess.run(["/tmp/vuc-binutils215/g++-"+snap+"-as215","-O2",
          "-c",str(path),"-o",str(out)],capture_output=True,text=True)
    row={"variant":name,"snapshot":snap}
    if cp.returncode:row["compiler_error"]=cp.stderr[-370:]
    else:
      try:
        score=m.run(out,cfg)
        row.update(size=score["length"],matched=score["exact_bytes"],
                   exact=score["exact"],different_offsets=score["different_offsets"],
                   relocations=score["relocations"],literals=score["verified_literal_bytes"])
      except ValueError as ex:row["unverified"]=str(ex)
    rows.append(row);print(json.dumps(row,sort_keys=True),flush=True)
(O/"results.json").write_text(json.dumps(rows,indent=2)+"\n")

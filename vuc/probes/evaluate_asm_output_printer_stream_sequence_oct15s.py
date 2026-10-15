#!/usr/bin/env python3
"""Real AsmOutput TU: recover historical ostream expression boundaries.

Original 0x08049da0 uses ostream aligned-code then newline separately.
Original 0x08049db8 prints a newline then rejoins an outer branch.
All experiments retain the actual accepted AsmOutput TU predecessor methods,
and must pass full original-length+all relocations+combined literal ownership.
No padding, asm, or broad compiler-flag search.
"""
from pathlib import Path
import importlib.util,itertools,subprocess,json
R=Path(__file__).resolve().parents[1]
OUT=Path("/tmp/vuc-binutils215/asm-print-stream-expression-oct15s")
OUT.mkdir(exist_ok=True)
base=(R/"probes/AsmOutputPrintInstructionOriginalUnitOct15r.cpp").read_text()
base=base.replace('#include "../src/AsmOutputUnit.cpp"','#include "'+str(R/"src/AsmOutputUnit.cpp")+'"')
spec=importlib.util.spec_from_file_location("match",R/"scripts/match_probe.py")
matcher=importlib.util.module_from_spec(spec);spec.loader.exec_module(matcher)
cfg=R/"config/asm_output_print_instruction_real_unit_oct15r.json"
variants={
 "selected":base,
 "constant-split":base.replace('output << aligned << "\\n";','output << aligned; output << "\\n";'),
 "constant-split-stream":base.replace('output << aligned << "\\n";','output << aligned; std::ostream &stream=output; stream << "\\n";'),
 "raw-split":base.replace('output << text << "\\n";','output << text; output << "\\n";'),
 "constant-and-raw-split":base.replace('output << aligned << "\\n";','output << aligned; output << "\\n";').replace('output << text << "\\n";','output << text; output << "\\n";'),
 "constant-temporary-pointer":base.replace('output << aligned << "\\n";','const char *code=aligned; output << code; output << "\\n";'),
 "constant-redirect-expression":base.replace('output << aligned << "\\n";','std::ostream &textOut=(output << aligned); output << "\\n";'),
 "constant-independent-endl":base.replace('output << aligned << "\\n";','output << aligned; output << std::endl;'),
 "constant-separate-newline":base.replace('output << aligned << "\\n";','output << aligned; (output << "\\n");'),
 "raw-independent-endl":base.replace('output << text << "\\n";','output << text; output << std::endl;'),
 "endmacro-first-separated":base.replace('if (hasEndMacro)\n        output << "\\n";','if (hasEndMacro) { std::ostream &os=output; os << "\\n"; }'),
}
assert "constant-split" in variants and variants["constant-split"]!=base
rows=[]
for label,source in variants.items():
 path=OUT/(label+".cpp");path.write_text(source)
 for snap in ("jan25","dec06"):
  obj=OUT/(label+"-"+snap+".o")
  p=subprocess.run(["/tmp/vuc-binutils215/g++-"+snap+"-as215","-O2","-frename-registers","-c",str(path),"-o",str(obj)],capture_output=True,text=True)
  item={"label":label,"snapshot":snap}
  if p.returncode:item["error"]=p.stderr[-160:]
  else:
   try:
    result=matcher.run(obj,cfg)
    item.update(length=result["length"],score=result["exact_bytes"],relocations=result["relocations"],literals=result["verified_literal_bytes"],mismatch=result["different_offsets"])
   except ValueError as exc:item["error"]=str(exc)
  rows.append(item)
  print({k:v for k,v in item.items() if k!="mismatch"},flush=True)
(OUT/"results.json").write_text(json.dumps(rows,indent=2)+"\n")

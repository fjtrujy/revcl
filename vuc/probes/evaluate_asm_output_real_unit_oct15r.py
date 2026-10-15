#!/usr/bin/env python3
"""Co-compile original accepted AsmOutput TU predecessors and new print body.

Original ELF sequences AlignString, AsmOutput_TabConstants and
AsmOutput_PrintInstruction. The accepted real AsmOutputUnit.cpp already
reconstructs genuine predecessor methods. This experiment retains their
original source and adds the new source-level printer, without inventing
calls or padding. Score source-unit effect diagnostic independently of
strict whole-.rodata padding ownership.
"""
from pathlib import Path
import importlib.util,subprocess,json
R=Path(__file__).resolve().parents[1]
O=Path("/tmp/vuc-binutils215/asm-output-real-unit-oct15r")
O.mkdir(exist_ok=True)
spec=importlib.util.spec_from_file_location("match",R/"scripts/match_probe.py")
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
cfg=json.loads((R/"config/asm_output_print_instruction_partial_oct15r.json").read_text())
original=(R/"src/AsmOutputUnit.cpp").read_text()
probe=(R/"probes/AsmOutputPrintInstructionFreshOct15r.cpp").read_text()
needle="""class Instruction {
public:
    const char *ToString();
};"""
assert needle in original
unit=original.replace(needle,"""class Instruction {
public:
    MLGameString name;
    unsigned line;
    unsigned kind;
    char *ToString();
};""")
# This header owns the accepted AlignConstant/AlignString/TabConstants code.
# Only make the original +8 kind layout visible; no accepted tracked file
# is modified. Its genuine .rodata/callee references still need authentication.
unit=unit.replace('#include <fstream>','#include <fstream>\n#include "'+str(R/"src/MLGameString.h")+'"',1)
body=probe[probe.index("void AsmOutput_PrintInstruction("):]
units={
 "isolated": probe.replace('#include "../src/MLGameString.h"','#include "'+str(R/"src/MLGameString.h")+'"'),
 "real-owner-append":unit+"\n"+body,
 "real-owner-front":unit[:unit.index("void AsmOutput_TabConstants(")]+body+"\n"+unit[unit.index("void AsmOutput_TabConstants("):],
}
data,target=m.decomp.load()
ro=next(x for x in target["sections"] if x["name"]==".rodata")
code=next(x for x in target["sections"] if x["name"]==".text")
address=int(cfg["address"],16)
expected=data[code["offset"]+address-code["address"]:code["offset"]+address-code["address"]+cfg["size"]]
ranges=cfg["relocation_literals"][".rodata"]
source_strings=[]
literal_section=None
rows=[]
for recipe,src in units.items():
 cpp=O/(recipe+".cpp");cpp.write_text(src)
 for snapshot in ("jan25","dec06"):
  obj=O/(recipe+"-"+snapshot+".o")
  p=subprocess.run(["/tmp/vuc-binutils215/g++-"+snapshot+"-as215","-O2","-frename-registers","-c",str(cpp),"-o",str(obj)],capture_output=True,text=True)
  row={"owner":recipe,"compiler":snapshot}
  if p.returncode:row["error"]=p.stderr[-250:]
  else:
   binary=obj.read_bytes()
   sec,_=m.elf_sections(binary)
   content=m.section_bytes(binary,sec[".rodata"][1])
   mapping=[]
   # Only these actual literal strings are referenced by the printer.
   # No arbitrary padding remapping is accepted in this diagnostic path.
   for item in ranges:
    addr=int(item["address"],16)
    original_bytes=data[ro["offset"]+addr-ro["address"]:ro["offset"]+addr-ro["address"]+item["size"]]
    positions=[i for i in range(len(content)) if content.startswith(original_bytes,i)]
    if not positions:row["error"]="genuine literal missing: "+item["address"];break
    # The exact accepted prologue already has a newline ending inside its
    # long diagnostic string; the standalone PrintInstruction "+\\n" is
    # emitted *last* and is the one referenced by the new function.
    mapping.append({"offset":positions[-1],"size":len(original_bytes),"address":item["address"]})
   if "error" not in row:
    try:
     normalized, reloc=m.prepare_named_function(binary,cfg["function"],cfg["relocations"],address,{}, {".rodata":mapping})
     row["length"]=len(normalized);row["relocations"]=reloc
     row["code_matching_diagnostic"]=m.compare(normalized,expected)["exact_bytes"] if len(normalized)==cfg["size"] else None
    except ValueError as ex:row["error"]=str(ex)
  rows.append(row);print(row,flush=True)
(O/"results.json").write_text(json.dumps(rows,indent=2)+"\n")

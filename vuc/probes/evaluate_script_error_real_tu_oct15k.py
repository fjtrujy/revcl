#!/usr/bin/env python3
"""Verify script_error against its real compiler unit and all literal bytes.

VucCompilerUnit.cpp has a 1-byte empty currFileName literal before the
two script_error strings; the isolated selected candidate does not.
Map all three literals to named original ELF evidence without skipping
the extra byte or reusing guessed addresses.
"""
from pathlib import Path
import importlib.util
import json
import subprocess

ROOT=Path(__file__).resolve().parents[2]
VUC=ROOT/"vuc"
OUT=Path("/tmp/vuc-binutils215/script-real-tunit-oct15k")
OUT.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location("match",VUC/"scripts/match_probe.py")
match=importlib.util.module_from_spec(spec)
spec.loader.exec_module(match)

config=json.loads((VUC/"config/script_error_probe.json").read_text())
data=json.loads((VUC/"config/data_currFileName.json").read_text())
empty=data["relocations"]["0x0"]
assert empty["symbol"]==".rodata"
assert empty["original_section"]==".rodata"
assert empty["literal_hex"]=="00"
assert config["source"]=="vuc/probes/ScriptErrorRealUnitOct15k.cpp"
literals=config["relocation_literals"][".rodata"]
assert [(x["offset"],x["size"]) for x in literals]==[(0,1),(1,2),(3,3)]
assert literals[0]["address"]==empty["address"]
cfg=OUT/"real-tunit-config.json"
cfg.write_text(json.dumps(config,indent=2)+"\n")
rows=[]
for snap in ("jan25","dec06"):
    for flags,opts in {
        "j1":["-O2","-frename-registers","-falign-jumps=1"],
        "plain":["-O2"],
    }.items():
        obj=OUT/(snap+"-"+flags+".o")
        cmd=["/tmp/vuc-binutils215/g++-"+snap+"-as215",*opts,
             "-c",str(ROOT/config["source"]),"-o",str(obj)]
        cp=subprocess.run(cmd,capture_output=True,text=True)
        row={"snapshot":snap,"flags":flags}
        if cp.returncode:
            row["error"]=cp.stderr[-180:]
        else:
            try:
                s=match.run(obj,cfg)
                row.update(length=s["length"],matching=s["exact_bytes"],
                           relocations=s["relocations"],
                           literals=s["verified_literal_bytes"],
                           different_offsets=s["different_offsets"])
            except ValueError as e:
                row["error"]=str(e)
        rows.append(row)
        print(row,flush=True)
(OUT/"results.json").write_text(json.dumps(rows,indent=2)+"\n")

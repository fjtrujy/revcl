#!/usr/bin/env python3
"""Find original-length unresolved VUC functions naturally emitted by exact TUs.

Unlike optimizer sweeps, this mines already authenticated source owners in a
historical GCC exact gate. It only proposes candidates; the normal original
ELF relocation/literal/code matcher is still required before acceptance.
"""
from pathlib import Path
import argparse
import csv
import json
import subprocess

ROOT=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument("--objects",required=True,type=Path,
                    help="Lima guest object directory from match_candidates.py")
args=parser.parse_args()
targets={}
with (ROOT/"vuc/config/function_status.tsv").open() as fh:
    for row in csv.DictReader(fh,delimiter="\t"):
        if row["status"]!="EXACT" and int(row["size"]):
            targets[row["symbol"]]=(int(row["size"]),row["status"],row["address"])
records=[]
objects=list(sorted(args.objects.glob("*.o")))
for obj in objects:
    result=subprocess.run(["nm","-S","--defined-only",str(obj)],capture_output=True,text=True)
    if result.returncode:continue
    for line in result.stdout.splitlines():
        parts=line.split()
        if len(parts)<4 or parts[3] not in targets:continue
        try:
            found=int(parts[1],16)
        except ValueError:
            continue
        size,status,address=targets[parts[3]]
        if found==size:
            records.append({"address":address,"size":size,"status":status,
                            "symbol":parts[3],"object":obj.name,"section_symbol_type":parts[2]})
# Deduplicate a compiler object only by its exact file identity, and retain
# flags/manifests for later inspection instead of pretending a strict match.
records.sort(key=lambda r:(r["status"]!="UNRESOLVED",r["size"],r["symbol"],r["object"]))
out=args.objects/"unresolved-original-length-in-exact-units.json"
out.write_text(json.dumps(records,indent=2)+"\n")
print("inspected",len(objects),"objects",len(targets),"target symbols")
for row in records[:120]:
    print(row["status"],row["address"],row["size"],row["symbol"][:90],row["object"])
print("total original-length candidate instances",len(records),"ledger",out)

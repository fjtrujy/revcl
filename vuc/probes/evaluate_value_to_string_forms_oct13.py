#!/usr/bin/env python3
"""Reconstruct Value::ToString using real 2004 C++ switch source forms.

All candidate literal bytes are independently checked against authenticated
original ELF strings (including the 64-byte zero initializer). A candidate
is scored only with original-length machine code and strict relocation mapping.
The original code's block order need not equal lexical switch-arm order.
"""
from itertools import permutations
import importlib.util
import json
import struct
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TMP = Path("/tmp/vuc-binutils215/value-tostring-forms-oct13")
TMP.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location("matcher", ROOT / "vuc/scripts/match_probe.py")
matcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matcher)
source = (ROOT / "vuc/probes/value_to_string_oct13.cpp").read_text().replace(
    '"../src/MLGameString.h"', '"' + str(ROOT / "vuc/src/MLGameString.h") + '"')
start = source.index("    case 1:")
case3 = source.index("    case 3:", start)
case4 = source.index("    case 4:", case3)
end = source.index("    }\n    return MLGameString", case4)
arms = {1: source[start:case3], 3: source[case3:case4], 4: source[case4:end]}
alternatives = {}
for order in permutations((1,3,4)):
    alternatives["case-" + "".join(map(str, order))] = source[:start] + "".join(arms[x] for x in order) + source[end:]
fmt = '''        std::sprintf(buffer, "%f,%f,%f,%f",
                     values[0], values[1], values[2], values[3]);'''
assert fmt in source
for name, replacement in {
    "explicit-double": '''        std::sprintf(buffer, "%f,%f,%f,%f",
                     (double)values[0], (double)values[1],
                     (double)values[2], (double)values[3]);''',
    "local-format": '''        const char *format = "%f,%f,%f,%f";
        std::sprintf(buffer, format,
                     values[0], values[1], values[2], values[3]);''',
    "local-destination": '''        char *destination = buffer;
        std::sprintf(destination, "%f,%f,%f,%f",
                     values[0], values[1], values[2], values[3]);''',
    "local-four-values": '''        float first = values[0], second = values[1];
        float third = values[2], fourth = values[3];
        std::sprintf(buffer, "%f,%f,%f,%f",
                     first, second, third, fourth);''',
    "local-four-values-reverse": '''        float fourth = values[3], third = values[2];
        float second = values[1], first = values[0];
        std::sprintf(buffer, "%f,%f,%f,%f",
                     first, second, third, fourth);''',
    "local-four-doubles": '''        double first = values[0], second = values[1];
        double third = values[2], fourth = values[3];
        std::sprintf(buffer, "%f,%f,%f,%f",
                     first, second, third, fourth);''',
}.items():
    variant = source.replace(fmt, replacement)
    if name.startswith("local-"):
        variant = variant.replace("    case 4:\n", "    case 4: {\n")
        variant = variant.replace("        break;\n    }\n    return MLGameString", "        break;\n    }\n    }\n    return MLGameString")
    alternatives[name] = variant

flags = {
    "j1": ["-O2", "-frename-registers", "-falign-jumps=1"],
    "j2": ["-O2", "-frename-registers", "-falign-jumps=2"],
    "j4": ["-O2", "-frename-registers", "-falign-jumps=4"],
    "j8": ["-O2", "-frename-registers", "-falign-jumps=8"],
    "j4-nogcse": ["-O2", "-frename-registers", "-falign-jumps=4", "-fno-gcse"],
}
known_strings = {
    b"%d\0": "0x08072971",
    b"%f\0": "0x08076699",
    b"%f,%f,%f,%f\0": "0x08076690",
}

def rodata_file_bytes(path):
    binary = path.read_bytes()
    shoff = struct.unpack_from("<I", binary, 0x20)[0]
    shsize, count, string_index = struct.unpack_from("<HHH", binary, 0x2e)
    sec = [struct.unpack_from("<IIIIIIIIII", binary, shoff+i*shsize)
           for i in range(count)]
    text_offset, text_length = sec[string_index][4:6]
    shstrings = binary[text_offset:text_offset+text_length]
    for part in sec:
        name_off = part[0]
        section_name = shstrings[name_off:shstrings.find(b"\0", name_off)]
        if section_name == b".rodata":
            offset, length = part[4:6]
            return binary[offset:offset+length]
    raise ValueError("no .rodata section")

def literal_ranges(content):
    if not content.startswith(b"\0"*64):
        raise ValueError("incorrect zero initializer")
    ranges = [{"offset":0,"size":64,"address":"0x0807669c"}]
    at = 64
    seen = set()
    while at < len(content):
        end = content.find(b"\0", at)
        if end < 0:
            raise ValueError("unterminated format literal")
        fragment = content[at:end+1]
        if fragment not in known_strings or fragment in seen:
            raise ValueError("unexpected/duplicate literal " + repr(fragment))
        seen.add(fragment)
        ranges.append({"offset":at,"size":len(fragment),"address":known_strings[fragment]})
        at = end+1
    if seen != set(known_strings):
        raise ValueError("not all original format strings emitted")
    return ranges

base = json.loads((ROOT/"vuc/config/value_to_string_oct13_probe.json").read_text())
rank = []
for name, text in alternatives.items():
    src = TMP / (name+".cpp")
    src.write_text(text)
    for setting, opts in flags.items():
        obj = TMP / (name+"-"+setting+".o")
        cc = subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215", *opts, "-c",str(src),"-o",str(obj)],capture_output=True,text=True)
        if cc.returncode:
            print("COMPILE",name,setting,cc.stderr[-140:],flush=True)
            continue
        size_line = next((line for line in subprocess.check_output(["nm","-S",str(obj)],text=True).splitlines() if line.endswith("_ZNK5Value8ToStringEv")),"")
        size = int(size_line.split()[1],16) if size_line else -1
        if size != 221:
            rank.append((0,size,name,setting,"size"))
            continue
        try:
            mapping = literal_ranges(rodata_file_bytes(obj))
            cfg = dict(base)
            cfg["relocation_literals"] = {".rodata":mapping}
            cfg["source"] = str(src)
            manifest = TMP / (name+"-"+setting+".json")
            manifest.write_text(json.dumps(cfg,indent=2)+"\n")
            score = matcher.run(obj,manifest)
            rank.append((score["exact_bytes"],size,name,setting,score["different_offsets"][:12]))
            if score["exact"]:
                print("EXACT",name,setting,src,manifest,flush=True)
                raise SystemExit(0)
        except ValueError as err:
            rank.append((0,size,name,setting,str(err)[:115]))
    print("CHECKED",name,flush=True)
print("BEST")
for row in sorted(rank,reverse=True)[:35]:
    print(row)

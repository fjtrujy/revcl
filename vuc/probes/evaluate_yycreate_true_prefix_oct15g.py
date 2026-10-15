#!/usr/bin/env python3
"""Test real FlexLexer predecessor ownership of the 139-byte yy_create_buffer.

The original generated lexer emits yyrestart, yy_switch_to_buffer and
yy_load_buffer_state immediately before yy_create_buffer. These experiments
preserve that order where included, without fabricated register controls.
The candidate must retain original size, relocations and literal bytes.
"""
from pathlib import Path
import importlib.util
import itertools
import json
import subprocess

ROOT = Path(__file__).resolve().parents[2]
VUC = ROOT / "vuc"
OUT = Path("/tmp/vuc-binutils215/yycreate-true-prefix-oct15g")
OUT.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location("matcher", VUC / "scripts/match_probe.py")
matcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matcher)
config = VUC / "config/yy_create_buffer_probe.json"

create_src = (VUC / "probes/yy_create_buffer.cpp").read_text()
lexer_src = (VUC / "src/FlexLexer.cpp").read_text()
restart_src = (VUC / "src/FlexLexerRestart.cpp").read_text()

create_idx = create_src.index("yy_buffer_state *yyFlexLexer::yy_create_buffer(")
header = create_src[:create_idx]
create = create_src[create_idx:].replace("b->size", "b->reserved_0c").replace("b->own", "b->yy_is_our_buffer")
header = header.replace("    char *pos;", "    char *yy_buf_pos;")
header = header.replace("    int size;", "    int reserved_0c;")
header = header.replace("    int count;", "    int yy_n_chars;")
header = header.replace("    int own;", "    int yy_is_our_buffer;")
header = header.replace("    int reserved[4];", """    int yy_is_interactive_internal;
    int yy_fill_buffer;
    int yy_is_interactive;
    int yy_at_bol;""")
header = header.replace("    void yy_init_buffer(yy_buffer_state *, std::istream *);", """    void yy_init_buffer(yy_buffer_state *, std::istream *);
    void yyrestart(std::istream *);
    void yy_load_buffer_state();
    void yy_switch_to_buffer(yy_buffer_state *);
    void yy_flush_buffer(yy_buffer_state *);
    void yy_delete_buffer(yy_buffer_state *);
    void LexerOutput(const char *, int);
private:
    char *yytext_ptr_;
    char reserved1_[0x18];
    std::istream *input_;
    std::ostream *output_;
    yy_buffer_state *current_;
    char yy_hold_char_;
    char reserved_align_[3];
    int yy_n_chars_;
    char *yy_c_buf_p_;
    char reserved2_[8];
    int yy_did_buffer_switch_on_eof_;
    char reserved3_[0x28];
    unsigned yy_buffer_char_index_;""")

def extract(start, end=None):
    i = lexer_src.index(start)
    j = lexer_src.index(end, i + 1) if end else len(lexer_src)
    return lexer_src[i:j].strip()

methods = {
    "restart": restart_src[restart_src.index("void yyFlexLexer::yyrestart("):].strip(),
    "switch": extract("void yyFlexLexer::yy_switch_to_buffer(", "void yyFlexLexer::yy_init_buffer("),
    "load": extract("void yyFlexLexer::yy_load_buffer_state("),
    "output": extract("void yyFlexLexer::LexerOutput(", "void yyFlexLexer::LexerError("),
    "init": extract("void yyFlexLexer::yy_init_buffer(", "void yyFlexLexer::yy_flush_buffer("),
    "flush": extract("void yyFlexLexer::yy_flush_buffer(", "void yyFlexLexer::yy_delete_buffer("),
    "delete": extract("void yyFlexLexer::yy_delete_buffer(", "void yyFlexLexer::yy_load_buffer_state("),
}
original_order = ["output", "restart", "switch", "load"]
seqs = [
    [],
    ["restart"],
    ["switch"],
    ["load"],
    ["restart", "switch"],
    ["switch", "load"],
    ["restart", "load"],
    ["restart", "switch", "load"],
    ["output", "restart", "switch", "load"],
    ["init", "restart", "switch", "load"],
    ["delete", "restart", "switch", "load"],
    ["flush", "restart", "switch", "load"],
    ["output", "init", "restart", "switch", "load"],
    ["output", "delete", "restart", "switch", "load"],
]
flagsets = {
    "o2rename": ["-O2", "-frename-registers"],
    "o2": ["-O2"],
    "j1": ["-O2", "-frename-registers", "-falign-jumps=1"],
    "j4": ["-O2", "-frename-registers", "-falign-jumps=4"],
    "no-peephole": ["-O2", "-frename-registers", "-fno-peephole2"],
}
results = []
best = None
for index, (seq, (flagname, flags)) in enumerate(itertools.product(seqs, flagsets.items())):
    tag = f"{index:03d}-{'-'.join(seq) if seq else 'isolated'}-{flagname}"
    text = header + "\n" + "\n".join(methods[s] for s in seq) + "\n" + create
    cpp = OUT / (tag + ".cpp")
    obj = OUT / (tag + ".o")
    cpp.write_text(text)
    compiler = ["/tmp/vuc-binutils215/g++-jan25-as215", *flags, "-c", str(cpp), "-o", str(obj)]
    run = subprocess.run(compiler, capture_output=True, text=True)
    if run.returncode:
        results.append({"tag": tag, "error": run.stderr[-240:]})
        continue
    try:
        match = matcher.run(obj, config)
    except ValueError as error:
        results.append({"tag": tag, "error": str(error)})
        continue
    record = {
        "tag": tag, "length": match["length"],
        "matched": match["exact_bytes"], "relocations": match["relocations"],
        "literals": match["verified_literal_bytes"],
        "different_offsets": match["different_offsets"],
    }
    results.append(record)
    if (record["length"], record["relocations"], record["literals"]) != (139, 5, 44):
        continue
    if best is None or record["matched"] > best["matched"]:
        best = record
        (OUT / "best.cpp").write_text(text)
        (OUT / "best.o").write_bytes(obj.read_bytes())
        (OUT / "best.flags").write_text(" ".join(flags) + "\n")
        print("BEST", json.dumps(best), flush=True)
    if record["matched"] == 139:
        break
(OUT / "results.json").write_text(json.dumps(results, indent=2) + "\n")
print(json.dumps({"tested": len(results), "best": best,
                  "valid": sum(x.get("length") == 139 and x.get("relocations") == 5 and x.get("literals") == 44 for x in results)},
                 indent=2))

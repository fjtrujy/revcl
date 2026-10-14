#!/usr/bin/env python3
"""Test source-level CFG and lifetime alternatives for three VUC partials.

Every candidate is historical C++03 compiled by the recovered GCC 3.3.3/
GNU as 2.15 wrapper. Generated source/object files stay outside the checkout.
Only original-length functions are submitted to the relocation-aware matcher.
"""
import importlib.util
import itertools
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TMP = Path("/tmp/vuc-binutils215/structural-control-flow-oct14")
TMP.mkdir(parents=True, exist_ok=True)
COMPILER = "/tmp/vuc-binutils215/g++-jan25-as215"
spec = importlib.util.spec_from_file_location("vuc_match", ROOT / "vuc/scripts/match_probe.py")
matcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matcher)

def measure(family, name, source, symbol, size, config, flags):
    src = TMP / (family + "-" + name + ".cpp")
    src.write_text(source)
    rows = []
    for key, options in flags.items():
        obj = TMP / (family + "-" + name + "-" + key + ".o")
        run = subprocess.run([COMPILER, *options, "-c", str(src), "-o", str(obj)],
                             capture_output=True, text=True)
        if run.returncode:
            rows.append((0, -1, name, key, "compile: " + run.stderr[-100:]))
            continue
        nm = subprocess.check_output(["nm", "-S", str(obj)], text=True)
        line = next((x for x in nm.splitlines() if x.split()[-1:] == [symbol]), "")
        length = int(line.split()[1], 16) if line else -1
        if length != size:
            rows.append((0, length, name, key, "size"))
            continue
        try:
            score = matcher.run(obj, ROOT / config)
            rows.append((score["exact_bytes"], length, name, key,
                         score["different_offsets"][:9]))
            if score["exact"]:
                print("NEW_EXACT", family, name, key, src, flush=True)
        except ValueError as exc:
            rows.append((0, length, name, key, "reject: " + str(exc)[:95]))
    return rows

COMMON = ["-O2", "-frename-registers"]
FLAGS = {
    "j1": COMMON + ["-falign-jumps=1"],
    "j4": COMMON + ["-falign-jumps=4"],
    "j8": COMMON + ["-falign-jumps=8"],
}

# 1. Recover source-level return and switch topology for operator+.
arithmetic = (ROOT / "vuc/src/ValueArithmeticUnit.cpp").read_text()
plus_start = arithmetic.index("Value Value::operator+(")
plus_end = arithmetic.index("Value Value::operator-(", plus_start)
plus = arithmetic[plus_start:plus_end]
branch_start = plus.index("    if (TypeConvert(")
body = plus[:branch_start]
switch = plus[plus.index("        switch (result.type)"):plus.rindex("    }\n    return result;")]
cases = switch[switch.index("        case 1:"):switch.index("        }")]
plus_forms = {
    "baseline": plus,
    "early-convert-return": body +
        "    if (!TypeConvert(*this, other, result, temporary)) return result;\n" +
        "    switch (result.type) {\n" + cases + "    }\n    return result;\n}\n",
    "early-convert-explicit-false": body +
        "    if (TypeConvert(*this, other, result, temporary) == false) return result;\n" +
        "    switch (result.type) {\n" + cases + "    }\n    return result;\n}\n",
    "switch-default-early": body +
        "    if (TypeConvert(*this, other, result, temporary)) {\n"
        "        switch (result.type) {\n" + cases +
        "        default: return result;\n        }\n    }\n    return result;\n}\n",
    "return-arms": body +
        "    if (TypeConvert(*this, other, result, temporary)) {\n"
        "        switch (result.type) {\n" +
        cases.replace("break;", "return result;") +
        "        }\n    }\n    return result;\n}\n",
    "shared-exit-label": body +
        "    if (!TypeConvert(*this, other, result, temporary)) goto finished;\n"
        "    switch (result.type) {\n" + cases +
        "    }\nfinished:\n    return result;\n}\n",
}
# Lexically reorder the independently terminating cases. This is not a
# machine-level branch-order override: the C++ switch remains genuine.
case1 = "        case 1: result.bits += temporary.bits; break;\n"
case3 = "        case 3: result.values[0] += temporary.values[0]; break;\n"
case4 = cases[cases.index("        case 4:"):]
for first, order in (("vector-first", (case4, case1, case3)),
                     ("scalar-first", (case3, case1, case4)),
                     ("int-last", (case4, case3, case1))):
    plus_forms[first] = body + (
        "    if (TypeConvert(*this, other, result, temporary)) {\n"
        "        switch (result.type) {\n" + "".join(order) +
        "        }\n    }\n    return result;\n}\n")
results = []
for name, fragment in plus_forms.items():
    source = arithmetic[:plus_start] + fragment + arithmetic[plus_end:]
    results += [("plus", *r) for r in measure(
        "plus", name, source, "_ZNK5ValueplERKS_", 219,
        "vuc/config/value_arithmetic_plus_probe.json", FLAGS)]
    print("PLUS", name, flush=True)

# 2. Genuine x87 expression/lifetime forms in division's vector case.
division_source = (ROOT / "vuc/probes/ValueDivisionUnit.cpp").read_text()
start = division_source.index("        case 4:", division_source.index("Value Value::operator/("))
end = division_source.index("            break;", start) + len("            break;")
vector = division_source[start:end]
div_forms = {"baseline": vector}
div_forms["direct-division"] = "        case 4:\n" + "".join(
    "            result.values[%d] /= temporary.values[%d];\n" % (i, i) for i in range(4)) + "            break;"
div_forms["local-reciprocal"] = (
    "        case 4: {\n" + "".join(
        "            const float reciprocal%d = 1.0f / temporary.values[%d];\n"
        "            result.values[%d] *= reciprocal%d;\n" % (i, i, i, i)
        for i in range(4)) + "            break;\n        }")
div_forms["separate-reciprocals"] = (
    "        case 4: {\n" + "".join(
        "            const float reciprocal%d = 1.0f / temporary.values[%d];\n" % (i, i)
        for i in range(4)) + "".join(
        "            result.values[%d] *= reciprocal%d;\n" % (i, i) for i in range(4)) +
    "            break;\n        }")
div_forms["local-denominators"] = (
    "        case 4: {\n" + "".join(
        "            const float denominator%d = temporary.values[%d];\n"
        "            result.values[%d] *= 1.0f / denominator%d;\n" % (i, i, i, i)
        for i in range(4)) + "            break;\n        }")
div_forms["local-numerators"] = (
    "        case 4: {\n" + "".join(
        "            float numerator%d = result.values[%d];\n"
        "            result.values[%d] = numerator%d * (1.0f / temporary.values[%d]);\n"
        % (i, i, i, i, i) for i in range(4)) + "            break;\n        }")
div_forms["array-loop"] = (
    "        case 4:\n"
    "            for (int i = 0; i != 4; ++i) result.values[i] *= 1.0f / temporary.values[i];\n"
    "            break;")
div_forms["array-loop-division"] = (
    "        case 4:\n"
    "            for (int i = 0; i < 4; ++i) result.values[i] /= temporary.values[i];\n"
    "            break;")
for name, block in div_forms.items():
    source = division_source[:start] + block + division_source[end:]
    results += [("division", *r) for r in measure(
        "division", name, source, "_ZNK5ValuedvERKS_", 235,
        "vuc/config/value_division_partial_oct13.json", FLAGS)]
    print("DIVISION", name, flush=True)

# 3. Scanner ownership/EOF topology. Preserve the same real buffer API and
# externally observable semantics while varying alias lifetimes and joins.
flex = (ROOT / "vuc/probes/yy_get_next_buffer_partial_oct13.cpp").read_text()
flex_forms = {"baseline": flex}
flex_forms["move-direct-index"] = flex.replace(
    "    for (i = 0; i < number_to_move; ++i)\n        *(dest++) = *(source++);",
    "    for (i = 0; i < number_to_move; ++i)\n        dest[i] = source[i];")
flex_forms["move-while-count"] = flex.replace(
    "    for (i = 0; i < number_to_move; ++i)\n        *(dest++) = *(source++);",
    "    i = number_to_move;\n    while (i-- > 0) *(dest++) = *(source++);")
flex_forms["move-while-pointers"] = flex.replace(
    "    for (i = 0; i < number_to_move; ++i)\n        *(dest++) = *(source++);",
    "    char *end = dest + number_to_move;\n    while (dest != end) *(dest++) = *(source++);")
flex_forms["saved-buffer-outside-loop"] = flex.replace(
    "        while (num_to_read <= 0) {\n            yy_buffer_state *b = yy_current_buffer;",
    "        yy_buffer_state *b = yy_current_buffer;\n        while (num_to_read <= 0) {")
flex_forms["saved-buffer-ref"] = flex.replace(
    "            yy_buffer_state *b = yy_current_buffer;",
    "            yy_buffer_state &buffer = *yy_current_buffer;\n            yy_buffer_state *b = &buffer;")
flex_forms["early-eof-ternary"] = flex.replace(
    "    if (yy_current_buffer->yy_fill_buffer == 0) {\n        if (yy_c_buf_p - yytext_ptr == 1)\n            return 1;\n        else\n            return 2;\n    }",
    "    if (!yy_current_buffer->yy_fill_buffer)\n        return (yy_c_buf_p - yytext_ptr == 1) ? 1 : 2;")
flex_forms["early-eof-flipped"] = flex.replace(
    "    if (yy_current_buffer->yy_fill_buffer == 0) {\n        if (yy_c_buf_p - yytext_ptr == 1)\n            return 1;\n        else\n            return 2;\n    }",
    "    if (!yy_current_buffer->yy_fill_buffer) {\n"
    "        if (yy_c_buf_p - yytext_ptr != 1) return 2;\n"
    "        return 1;\n    }")
flex_forms["final-eof-condition"] = flex.replace(
    "    if (yy_n_chars == 0) {\n        if (number_to_move == 0) {",
    "    if (!yy_n_chars) {\n        if (!number_to_move) {")
flex_forms["final-eof-return"] = flex.replace(
    "    } else\n        ret_val = 0;\n    yy_n_chars += number_to_move;",
    "    } else { ret_val = 0; }\n    yy_n_chars += number_to_move;")
for name, code in flex_forms.items():
    results += [("flex", *r) for r in measure(
        "flex", name, code, "_ZN11yyFlexLexer18yy_get_next_bufferEv", 550,
        "vuc/config/yy_get_next_buffer_oct13_probe.json",
        {"j1": FLAGS["j1"], "j4": FLAGS["j4"]})]
    print("FLEX", name, flush=True)

for family in ("plus", "division", "flex"):
    print("BEST", family, flush=True)
    best = sorted((r for r in results if r[0] == family), reverse=True)[:16]
    for row in best:
        print(row, flush=True)

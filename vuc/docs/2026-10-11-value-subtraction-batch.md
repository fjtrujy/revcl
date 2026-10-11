# VUC exact subtraction and pending arithmetic batch

Date: 2026-10-11. Branch: `vuc`.

## Verified exact source

Reconstructed `Value::operator-(Value const&) const` at
`0x0806ea90` from period-plausible C++ in `vuc/src/ValueMinus.cpp`.
The 221-byte implementation matches **221/221 original instruction bytes**
with the January 25 2004 GCC 3.3.3 compiler snapshot and GNU as 2.15,
using `-O2 -frename-registers -falign-jumps=1`.

The source constructs two local `Value` objects, calls the original
`Value::TypeConvert()` at `0x0806f7d0` and dispatches on the signed
`type` discriminator. Type 1 subtracts one integer; type 3 subtracts
one float; type 4 subtracts four floats. The compiler produces all of
the original register assignments and branch padding naturally, with
no assembly or postprocessing. The original linked call is verified
through the relocation-aware gate.

The **complete accepted gate** checks **153/153 function records** and
**9,144/9,144 instruction bytes**, including **391 resolved ELF32
relocations** and **132 literal bytes** checked independently. The
full original ELF has **298 named text-function records**:
**153 EXACT, 10 EXPERIMENTAL and 135 UNRESOLVED**.

The other arithmetic, comparison and string-data hypotheses currently
in `vuc/probes/` remain experimental. Do **not** infer exactness from
length agreement, matching instruction prefixes or semantic similarity.

## Reproduction

```sh
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/coverage.py --write
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/resume-153
```

The full original executable still does **not** match byte-for-byte.
Recovering the authentic historical downstream Debian toolchain,
the remaining original C++ functions, sections, link order and ELF
metadata are mandatory future work. Per-function validation is only a
necessary incremental condition, never whole-binary acceptance.

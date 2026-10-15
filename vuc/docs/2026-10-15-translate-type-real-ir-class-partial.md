# VUC Translate(Type): original IR type constructors and 64/352 strict partial

Date: 2026-10-15. Original VUC 2004.1 Linux i386 executable.
Function `Translate(const Type&, unsigned, int)` at `0x0806a9b0`,
**352 bytes**. Previous original source-unit file
`vuc/src/TranslateTypeUnit.cpp` only matched the module's static
31-byte initialization wrapper; it contained a deliberately simplified
`IR::Data`/value/register body. That accepted startup translation
unit remains **untouched** in this batch.

## Original ELF class ownership

The original machine code references five distinct C++ RTTI/vtable
objects, whose original ELF symbols and authentic code-pointer targets
are established independently:

| Source object | Original vtable symbol | Vptr stored in object |
| --- | --- | --- |
| `IR::Int_Data` | `_ZTVN2IR8Int_DataE` | `0x08072510` |
| `IR::UInt_Data` | `_ZTVN2IR9UInt_DataE` | `0x08072520` |
| `IR::Float_Data` | `_ZTVN2IR10Float_DataE` | `0x080729f0` |
| `IR::Float4_Data` | `_ZTVN2IR11Float4_DataE` | `0x08072a00` |
| `IR::Array_Data` | `_ZTVN2IR10Array_DataE` | `0x08076180` |

The original `Translate` reads `Type::flags` at +0,
`Type::component` at +4, `Type::width` at +8, and an extra flag
byte at +15. It strips `0x3c00` from the initial kind, then
dispatches through kinds 1, 2, 3 and bitfields 0x100/0x400.

The actual original machine code constructs:

- an 8-byte UInt object, kind 1;
- an 8-byte Int object, kind 2, when kind 1 and the high trailing
  flag byte is zero;
- a 12-byte Float object, kind 4, with signed width shift by 10;
- a 16-byte Float4 object, kind 3, with channel/width values extracted
  from the flags, user options and allocation bits;
- a nested Float4 then 16-byte Array object, kind 0, with the inner
  object at offset +8 and component count at offset +12.

All ordinary C++03 `new` calls remain real, and the class
constructors use polymorphic C++ classes; no handwritten vtable
pointer assignments are used.

## Full original-sized source control

The fresh `vuc/probes/TranslateTypeFullOct15q.cpp` implements
these actual `IR::Type_Data` children and a native
`switch(kind)`, with `int kind` rather than unsigned to preserve
the original ELF's signed `jg` branch (not `ja`). It is compiled
with both recovered Jan25 and Dec06 historical GCC 3.3.3 snapshots,
GNU assembler 2.15, and the normal `-O2 -frename-registers` flags.

The strict matcher authenticates **original length 352 bytes**,
**all 12 original relocation sites** (six `operator new` calls
and six original vtable references, with Float4 used twice),
and **64/352 original-offset instruction bytes** in each snapshot.
There are no literal data spans to authenticate here.

**This is a low-byte-coverage EXPERIMENTAL source candidate, not an
exact function.** It is selected only in
`vuc/config/selected_candidates.json`; it is forbidden from
`exact_candidates.json` until all 352 code bytes match.

## Original dispatch source order experiments

The focused source generator
`vuc/probes/evaluate_translate_type_dispatch_oct15q.py` tests
ordinary if-chain, switch fallthrough and switch-return
implementations based on original control-flow evidence. A
signed switch-return naturally yields the original 352-byte
size; a signed initial kind matches the original signed branch
sense. The switch-fallthrough form instead yields 336 bytes.
The subsequent
`vuc/probes/evaluate_translate_type_switch_ownership_oct15q.py`
compares six legitimate C++03 case orders, an original-style shared
UInt label, and a separate conditional dispatch. All switch
reorderings and the explicit shared label compile to exactly
**352 bytes**, with unchanged **64/352** matching code and all
12 relocations under both historical snapshots. A separate if-chain
gives 338 bytes, failing the original-size requirement.

This demonstrates that simply permuting case labels does not
restore the original allocation block placement or register
ownership. More promising next work is to reconstruct the
original `Translate.cpp` predecessor source, original inlined
IR constructor definitions and neighboring type semantics.
Do not run equivalent switch case-order sweeps again.

All generated ELF32 compiler objects and negative-control JSON
remain under
`/tmp/vuc-binutils215/translate-type-source-dispatch-oct15q/` and
`/tmp/vuc-binutils215/translate-switch-ownership-oct15q/`
in the existing Lima guest.

Reproduce:

```sh
R=/path/to/revcl/vuc
/tmp/vuc-binutils215/g++-jan25-as215 \
  -O2 -frename-registers \
  -c "$R/probes/TranslateTypeFullOct15q.cpp" \
  -o /tmp/vuc-binutils215/translate-type-full-oct15q.o
python3 "$R/scripts/match_probe.py" \
  --object /tmp/vuc-binutils215/translate-type-full-oct15q.o \
  --config "$R/config/translate_type_partial_oct15q.json"
```

The status after the companion BroadcastMove partial is
**204 EXACT / 34 EXPERIMENTAL / 60 UNRESOLVED** over the original
298-function list. There is still no whole linked VUC ELF match.

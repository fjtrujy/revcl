# VUC Type stream input: authenticated 369-byte original-length partial

Date: 2026-10-15. Original ELF: VUC 2004.1 Linux i386, original function
`std::istream &operator>>(std::istream&, Type&)` at `0x0804f2a0`.

## Finding

The source-derived Type streaming input control-flow reconstruction already
present in pre-existing **untracked** developer scratch files was checked
against the original ELF with Jan25 and Dec06 GCC 3.3.3 snapshots and GNU
binutils 2.15. It naturally emits a function of **369 bytes**, precisely
the original size, matching **264/369 original-offset instruction bytes**,
**all 10 relocation sites** and **48 source literals authenticated against
the original ELF**. The seven-element local `static const char *tn[]`
array resolves to the authenticated original `.data` object at `0x0807700c`.

This is the first selected, relocation- and literal-gated candidate for the
previously UNRESOLVED stream operator. It is now strictly EXPERIMENTAL, not
accepted into `vuc/src` or the exact manifest. The inventory advances
to **204 EXACT / 27 EXPERIMENTAL / 67 UNRESOLVED** over 298 original
functions. Do not mistake this for 264 newly accepted exact bytes.

The original scratch source and config remain untracked and unchanged.
The independently tracked copies are
`vuc/probes/TypeStreamInputOriginalUnitOct15n.cpp` and
`vuc/config/type_stream_input_partial_oct15n.json`. They preserve
the original 16-byte Type local, the seven original type names, the
imperfect/quirky `"const"` handling and the original signed/unsigned
or string operations. No original data is copied into executable text.

## Where the 105 mismatches concentrate

The compiled objects and original disassembly show two coherent regions:

1. The **first 145 instruction bytes**: stack-local Type construction,
   zero initialization and copy into the destination Type. The original
   creates its 16-byte local with first fields initialized before a final
   `kind` write and then copies the fields through registers. The current
   candidate hoists a text-buffer pointer earlier and directly folds
   zero constants into destination stores. The mismatches are from natural
   historical GCC optimizer scheduling and lifetime decisions.
2. At `+145` through `+343`, a long consecutive interval is byte
   identical after original relocation normalization. The original parser
   searches each type name with `strstr`, handles `float4` specially,
   finds brackets with `strchr`, parses a dimension with
   `__strtol_internal`, and retains the original peculiar `const`
   behavior. Residual suffix differences occupy isolated original-offset
   branch/register bytes.

The grouped mismatching original offsets are:

```
+11..31, +33..36, +40, +44, +46, +48, +50..56,
+60..62, +67..95, +97..112, +114..122, +124..126,
+129, 136, 142, 145, 344, 347, 353, 357, 365
```

## A bounded historical C++03 constructor experiment

`vuc/probes/evaluate_type_input_local_constructor_oct15n.py` tests
ordinary C++03 default-constructor initialization lists and assignment
bodies (including deliberately non-idiomatic ordering), keeping the
original `Type` field layout and original compiler options fixed:

- Nine constructor spelling/field-init order variants with implicit Type
  assignment each yield the original **369-byte size** and **264/369**
  matching original-offset bytes. GCC folds all into the same code.
- User-written per-member assignment increases the helper to **385 bytes**
  and is rejected on the original-size gate.
- A grouped flags-word copy remains **369 bytes** but yields only
  **251/369** or **252/369** in most forms; some compile to the baseline
  **264/369**. None is better.

The complete per-case ledger and generated ELF32 objects reside in
`/tmp/vuc-binutils215/type-input-local-ctor-oct15n/results.json` in
the existing Lima guest. This rules out repeated default-initializer
lexical variations as an effective route. More promising future work is
to reconstruct the original *declared Type copy/assignment semantics*
from independent class layout and accepted source-unit evidence,
or restore its genuine translation unit, rather than attempting more
compiler-flag permutations.

Original linked whole-ELF byte identity remains unestablished.

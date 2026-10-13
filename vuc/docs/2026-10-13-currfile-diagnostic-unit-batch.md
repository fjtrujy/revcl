# VUC `currFileName` global initializer: genuine diagnostic source context

Date: 2026-10-13. Branch: `vuc`.

## New exact 31-byte function

The original `_GLOBAL__I_currFileName` at `0x08071580` was previously
**29/31** under the recovered January 2004 GCC 3.3.3 / GNU as 2.15
compiler. A real VUC compiler-unit context now reproduces **31/31
instruction bytes**, with its one original relocation to
`__static_initialization_and_destruction_0(int,int)` at `0x0806f9f0`.
The accepted source is `vuc/src/VucCompilerUnit.cpp`; the exact-match
configuration is `vuc/config/iostream_currfile_real_context_oct13.json`.
Compilation uses `-O2 -frename-registers -fuse-cxa-atexit
-ffunction-sections` and contains no assembly, artificial padding,
forged relocations or invented functions.

## What changes the register selection

The original application's named methods adjacent to this initializer
are `script_lex()` at `0x0806fa50`, `script_error(char const*)` at
`0x0806fa80`, `ParseFile()`, `ExportSymbols()`, and `main()` at
`0x08071530`. We reconstructed an ordinary C++ compilation unit using
the previously verified real `script_lex` and `main` bodies, and the
natural C++ diagnostic formatting of `script_error` from the existing
partial probe.

The successive controls show:

| Genuine source neighbors | Wrapper comparison |
| --- | ---: |
| Original-style globals only | 29/31 |
| Real `main()` | 29/31 |
| Real `script_lex()` and `main()` | 29/31 |
| Real `script_lex()`, `script_error()`, and `main()` | **31/31** |

Under this combination, the already-accepted `script_lex` remains
**39/39** with three verified relocations, and `main` remains
**63/63** with three relocations. These are NOT two newly accepted
functions; their original addresses were already in the strict
manifest. The original `script_error` remains **non-exact**: the
combined candidate compiles to 174 bytes versus the original 161.
The diagnostic C++ is useful source-level context, not a fully
reconstructed Bison/compiler translation unit.

**Important limitation:** the 88-byte static initialization helper
is only **77/88** when compiled alongside the authentic neighbor
methods, while it remains **88/88** from the existing independent
accepted `vuc/src/CurrFileInitialization.cpp` source. We therefore
preserve its accepted manifest unchanged. Neither a correct wrapper
nor a group of matching application functions proves whole-source-unit
or linked-ELF equality. Further source-order recovery must reconcile
the current GCC register allocation of this local initializer.

## Reproducible, separated experiments

- `vuc/probes/curr_file_with_main_oct13.cpp`: unchanged 29/31;
- `vuc/probes/curr_file_with_real_neighbors_oct13.cpp`: unchanged 29/31;
- `vuc/probes/curr_file_with_diagnostics_oct13.cpp`: exact wrapper;
- `vuc/config/script_error_compiler_unit_oct13_probe.json`: checks
  the combined candidate's literal data using original NUL,
  `"("`, and `"):"` locations, but still rejects the 174-byte
  nonmatching diagnostic body.

## Strict acceptance

The accepted original-function count is now **173/298**, comprising
**11,708/11,708** independently checked instruction bytes: 173 EXACT,
6 EXPERIMENTAL, 119 UNRESOLVED. That does not establish the final
whole-ELF match.

```sh
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/coverage.py --write
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/exact-gate-173-currfile-oct13
```

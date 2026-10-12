# VUC pending-probe inventory and register-selection investigation

Date: 2026-10-11; branch `vuc`.

## Independently rechecked exact baseline

The recovered Jan 25, 2004 GCC 3.3.3 prerelease compiler and GNU as 2.15
revalidated the **162/162 accepted original function-symbol records**,
**10,927/10,927 instruction bytes** and **409 resolved relocations**.
This is **not** byte identity of the complete original VUC ELF. The
298-symbol original text inventory remains **162 EXACT, 9 EXPERIMENTAL,
127 UNRESOLVED**, excluding no named source merely because it is difficult.

## Source/config diagnostic batch

`vuc/scripts/recheck_pending_probes.py` compiles pending C++ hypotheses
with the same historical compiler and passes their real ELF32 objects to
the original-relocation-aware strict named-function verifier.

The initial scan found 44 pending config files, including two alternate
configs for **already accepted** functions:
`list_dispatch_bind_var_probe.json` (57/57) and
`ml_string_table_cleanup_probe.json` (22/22). It would be misleading
to count these as two newly reconstructed functions. The new scanner
therefore excludes identities **by original mangled symbol plus original
ELF virtual address**, not merely by manifest pathname. Host tests
verify both alternate configurations are excluded without compiling.

With that identity guard applied, **35 pending configurations were
checked and none supplied a new exact original function**. Compilations
that emitted the wrong function length or lacked grounded literal
locations are recorded as UNAVAILABLE, never promoted.
Results are emitted under the caller-specified scratch directory
(`pending_probe_results.json`) and do not alter project files.

## Improved ordinary-C++ diagnostics, not accepted matches

Original `script_error(char const*)` is **161 bytes**. Its disassembly
starts two distinct output expressions: print the current source
filename, then restart output from `std::cout` to print `(`, the
line number, `):`, the message and newline. Expressing this as two
statements in the provisional source, with `-O2 -frename-registers
-falign-jumps=1`, reproduces the function length and **155/161
instruction bytes**, resolving all 15 relocations and all five
literal bytes. Six nonmatching bytes at original offsets
`42, 48, 99, 105, 131, 137` encode register choices for literal
addresses and their argument stores. Natural source variations,
including explicitly named separator strings, local stream
references, independent expressions and declaration qualifiers,
retained the same mismatches.

`Print(ASTFunCall*)` produces **96/98** bytes when its original
argument and literal references are checked with **`-O2` without
`-frename-registers`**. The only differences are offsets
`23, 29`, again the temporary register used for the callee field
load and outgoing argument store. This improves its tracked probe
over the prior default `-frename-registers` (92/98).

`Canonize2_Reduce()` produces **83/85** bytes under `-O2
-frename-registers -falign-jumps=1`, with three resolved relocations.
This is an ordinary C++ reconstruction of the early-exit guard,
`globalEnv.clear()` and fragment loop, but two bytes of the
initial register selection remain unverified.

`Value::operator*()` remains **219/222**, with differences confined
to original offsets `155, 162, 164`: the candidate allocates ECX
for the single integer multiplication while the original uses EDX.
Additional ordinary source expressions and compilation-unit ordering
(operator+ before/after, multiplication alone, and signed/unsigned
intermediates) did not change these bytes.

Both separately recovered **2003-12-06** and **2004-01-25** historical
GCC 3.3.3 prereleases produced the same near-match counts for
`script_error()`, `Print(ASTFunCall*)` and `Value::operator*()`
under their best checked source/flag variants. Neither is authenticated
as the original patched Debian GCC 3.3.3 `0pre3` toolchain.

An additional **70 historical-compiler/optimization configurations**
(35 single-option configurations with each of the December and January
GCC snapshots) produced no exact `script_error()`. The best still
has the same six register-encoding differences at the same offsets.
The `Value::operator*()` two-flag combination sweep also produced no
222/222 match; the best remains 219/222 with ECX rather than EDX for
the integer multiplication. Incompatible 2004 compiler flags are
reported as unavailable, not equivalent successful tests.

For the specific header versions used in these C++ probes, the
**January 2004 GCC source tree and the later Debian GCC 3.3.5
compatibility-header package have byte-identical copies** of
`std_ostream.h`, `ostream.tcc`, `ios_base.h`, `std_ios.h`,
`std_istream.h`, `std_iostream.h`, `basic_ios.h`,
`basic_string.h`, `basic_string.tcc`,
`stl_vector.h`, `stl_uninitialized.h`, and `stl_iterator.h`.
This removes a specific suspected header mismatch for the tested
subsystems; it does **not** authenticate the historical Debian compiler
patchset, complete target sysroot or linking environment.

The flags in the tracked exploratory manifests have been updated to
reflect these independently confirmed best forms. All files remain
under `vuc/probes` and none enters the accepted `vuc/src` or
`vuc/config/exact_candidates.json` set. A non-exact match is a
failure of the mandatory byte-exact acceptance contract.

## Continuation: reconstructed translation units and pointer arithmetic

The follow-up `evaluate_script_error_tu.py` combines the accepted
`script_lex()` function and the experimental `script_error()` using
both source-definition orders. With the January 2004 compiler and
`-O2 -frename-registers -falign-jumps=1`, both combined units
retain the already accepted **39/39** for `script_lex()`, while
`script_error()` remains **155/161** with exactly the same six
register-encoding differences. Omitting `-frename-registers` changes
their function lengths (38 and 160) and is non-exact. This rules out
that specific source neighbor/order as the cause of the diagnostic
register discrepancy.

`evaluate_align_string_shapes.py` tests ten source-level variants
of the original 145-byte `AlignString()` function: pointer offset
recomputation, loop order, local pointer lifetimes and conventional
`for` / `while` forms. None reproduces the original function
length; the best original-shaped 142-byte reconstruction remains
experimental. The original's final `strcat()` pointer offset is
materially different from the compiler's optimized form, so an
ordinary rewrite alone did not reproduce its register allocation.

Both follow-up scripts compile scratch objects in the Linux VM,
resolve actual original symbols and reject partial matches. They do
not alter the exact accepted source set.

## Reproduction

```sh
python3 -m unittest discover -s vuc/tests -q
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/exact-gate-newbatch-162

limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/recheck_pending_probes.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/recheck-20261011-fixed
```

Additional source and flag sweeps are reproducible using:
`evaluate_value_mul_source_context.py`,
`evaluate_value_mul_flag_pairs.py`,
`evaluate_script_error_tu.py`,
`evaluate_align_string_shapes.py`,
`evaluate_script_error_source_order.py` and
`evaluate_script_error_flags.py`.

Next investigate exact original translation-unit context and original
Debian toolchain patches before revisiting persistent register
allocation differences. **Whole-ELF byte-for-byte identity remains
mandatory, and has not been achieved.**

# VUC historical source controls: Value case order, argument path copies, and ifstream state

Date: 2026-10-16. Branch: `vuc`. Original target: the tracked VUC 2004.1 Linux i386 ELF32 archive. Historical compiler: recovered January 25, 2004 GCC 3.3.3 and GNU binutils 2.15 in the existing Lima `revcl-oracle` VM. The experiments below use ordinary C++03 source, not patched machine code, fabricated relocations, assembly directives, or filler.

## 1. Value::operator+ original-length case-order controls

Original `_ZNK5ValueplERKS_` at `0x0806e9b0`, 219 instruction bytes. The selected candidate `vuc/probes/ValueAdditionLaneOrderOct15.cpp` and existing `vuc/config/value_addition_partial_oct14_probe.json` match **203/219 bytes**, including the authentic `Value::TypeConvert` call relocation. Its residual mismatches are at `+169,+172,+175,+194..+206`.

`vuc/probes/evaluate_value_plus_case_order_oct16.py` tests genuine source-level switch-case order permutations, keeping the existing branch semantics and predecessor method definitions. Under `-O2 -frename-registers -falign-jumps=1`:

- The baseline and all five permutations of cases 1/3/4 with the existing lexical vector-lane order 3,1,2,0 compile to 219 bytes and reproduce **203/219**; the strict mismatch-offset set is unchanged.
- Four permutations using ascending lexical vector-lane order 0,1,2,3 compile to **216 bytes**, so they are excluded from original-length partial comparison.

The historical GCC switch lowering canonicalizes these changes: simply rearranging switch source labels does not remove the optimizer's cross-jump/tail sharing with the scalar lane. A new experiment must change a *genuine code-generation cause*, rather than repeat case permutations. No exact promotion or manifest modification is justified.

Reproduce in the Linux VM:

```sh
python3 /path/to/revcl/vuc/probes/evaluate_value_plus_case_order_oct16.py
```

## 2. ParseArguments genuine path-copy idioms

Original `_Z14ParseArgumentsiPPc` at `0x0805b3e0`, **777 bytes**. The checked-in `ParseArgumentsOriginalBranchesOct16.cpp` contains previously recovered filename validation and explicit/implicit output branching, but its ordinary historical build remains **713 bytes**. The original at `0x0805b51f` performs a variable-length `rep movsl` with byte tails when copying the input-directory prefix.

`vuc/probes/evaluate_parseargs_pathcopy_oct16.py` tests nine normal source idioms for that copy, each with `-O2 -frename-registers` and optionally `-falign-jumps=8`: `memcpy`, `memmove`, `__builtin_memmove`, `std::copy`, `std::copy_backward`, `std::char_traits<char>::copy/move`, an indexed character loop and a pointer-iterator character loop. The historical compiler gives:

| Source family | Default jump alignment | `-falign-jumps=8` |
| --- | ---: | ---: |
| `memcpy`, `memmove`, `std::copy`, `std::copy_backward`, `char_traits::copy/move`, pointer iterator | 713 | 697 |
| Indexed character loop | 697 | 689 |
| `__builtin_memmove` | Compile error (not declared by this historical compiler configuration) | Compile error |

These are **length results only**, not original-offset scores. None has the original 777-byte length, so none is eligible for EXPERIMENTAL selection; no relocations or string ownership are falsely certified. The separate earlier `-minline-all-stringops` experiment produced 1081/1056 bytes and also remains ineligible. A future attempt should identify the specific original source-level construct or translation-unit setting that emitted the `rep movsl` sequence *without* globally inlining unrelated string calls.

A follow-up historical-GCC diagnostic, `evaluate_parseargs_builtin_copy_oct16.py`, keeps ordinary libc string calls external using `-fno-builtin` while retaining an explicit `__builtin_memcpy` for the directory-prefix copy under `-minline-all-stringops`. This is a **compiler-mechanism control**, not an authenticated original Makefile flag. It produces 784 bytes with jump alignment 1 and default loop alignment. With explicit loop alignment 1, 2, 4 and 8, the candidate measures 771, 772, 772 and 776 bytes, respectively (jump alignment 1, label alignment 1). Further jump/loop configurations range from 776 to 794 bytes. None of these tested outputs equals the original 777-byte function.

Crucially, the closest 776-byte object is **not structurally near exact** merely because its total length differs by one byte. Its path-copy block contains an extra source-copy loop and a subsequent `cld; rep movsl`, whereas the original at `0x0805b51f` enters a direct `cld; rep movsl` with conditional word/byte tails. The scratch object's extra loop begins near relative `+0x128`, with `rep movsl` only near `+0x18b`; the original copy begins at relative `+0x13f`. This is specific negative evidence against promoting the flags-only candidate, even if another alignment happens to produce 777 bytes. No original-length strict partial is claimed.

Reproduce:

```sh
python3 /path/to/revcl/vuc/probes/evaluate_parseargs_pathcopy_oct16.py
python3 /path/to/revcl/vuc/probes/evaluate_parseargs_builtin_copy_oct16.py
```

## 3. TryOpenFile state-predicate contract

Original `_Z11TryOpenFilePKc` at `0x0804ca30`, **754 bytes**. The existing selected `TryOpenFileSuccessFirstOct15t.cpp` reproduces **650/754 bytes** with its authenticated C++ stream lifetime, relocations, and original length, under `-O2 -frename-registers -falign-jumps=8`.

`vuc/probes/evaluate_tryopen_predicates_oct16.py` checks eight ordinary C++03 stream-state predicates against the *existing strict original ELF manifest*, including length and relocation resolution:

| Source predicate | Result |
| --- | --- |
| `!f.fail()`, `f`, `!(!f)` | 754-byte size, **650/754** matching |
| `f.rdstate() & std::ios::failbit` | 754-byte size, **649/754** matching |
| `f.good()`, `!f.bad() && !f.fail()`, `f.rdstate() == 0` | Wrong size: 762 vs 754 |
| `f.is_open()` | Extra unresolved `__basic_file::is_open()` relocation, ineligible |

The compiler reduces the boolean-stream forms to the same state-bit test; no byte progress came from these predicates. `is_open()` does not have an authenticated relocation in the current original function. The selected 650-byte partial remains stronger than every eligible result and is preserved unchanged.

Reproduce:

```sh
python3 /path/to/revcl/vuc/probes/evaluate_tryopen_predicates_oct16.py
```

## Acceptance boundary

These are **new reproducible negative controls**, not newly byte-exact functions and not a claim of increased original-offset matching bytes. The accepted exact manifest and selected experimental candidates are intentionally unchanged. Do not use the wrong-sized parser or the extra-relocation stream form as scored partials. Future work should prioritize underexplored authentic original-sized candidates or real translation-unit/RTL causes, retaining all accepted exact functions, initialized data, and BSS declarations.

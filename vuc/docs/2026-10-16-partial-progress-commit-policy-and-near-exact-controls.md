# Preserve useful VUC partial-byte progress at each checkpoint

Date: 2026-10-16. Branch: `vuc`. The independently reconstructed VUC 2004.1 Linux x86 ELF has a separate **byte-exact acceptance gate**, but meaningful **original-sized, relocation-authenticated partial matches and unsuccessful source experiments must also be committed**. An experiment's failure to reach full equality does not make its reproducible source, score, and negative controls disposable.

## What was already committed

- `ListFile::WriteToFile(const char*)` at `0x0805abc0`: genuine C++03 stream source, **875/1071** original-offset bytes, **58 authentic relocations**, and three authenticated literal bytes. Committed in `ae6dea65` after the recent rebase (original pre-rebase commit `8be83941`). **EXPERIMENTAL**, not EXACT.
- `TryOpenFile(const char*)` at `0x0804ca30`: genuine C++03 ifstream source, **650/754** original-offset bytes, original length and authentic relocation mappings. Committed and entered into the experimental manifest in `a11b52eb`, alongside an update recording `ListFile::WriteToFile` as experimental.
- The independently reproduced original-length `std::vector<Type>` insertion and other existing experimental candidates retain their source/config in `vuc/probes/` and `vuc/config/`; the selected queue tracks them without weakening `exact_candidates.json`.

The latest host regression for that checkpoint passed **193 tests**. The exhaustive source-function inventory then stood at **204 EXACT / 38 EXPERIMENTAL / 56 UNRESOLVED**. These are per-function claims, **not** evidence of whole-ELF byte equality.

## Preserve this new experiment batch, even without a new exact match

The following reproducible C++03 source-level evaluators were authored in the next investigation and are now recorded as a separate commit rather than being abandoned as untracked scratch files:

- `evaluate_instruction_cleanup_owner_lifetime_oct15v.py`: probes ordinary pointer/reference/loop-lifetime representations for the compiler-generated Instruction arena cleanup `__tcf_1` at `0x08051980`, original **46 bytes**. Historical January/December GCC 3.3.3 baseline and several tested source forms retain **45/46**, differing at the disposable stack-slot pop-register byte at offset `+42`. A different register byte is still not an exact match.
- `evaluate_print_fun_call_return_contract_oct15v.py`: tests the real `Print(ASTFunCall*, std::ostream&)` source unit with plausible `void`, ostream-reference, pointer, and other return contracts (the return type is not encoded in its mangled symbol). The ordinary `void` baseline with `-O2` remains **96/98**; a tested ostream-reference return expression also remains **96/98**, with mismatch offsets `+23` and `+29`.
- `evaluate_asm_prologue_return_contract_oct15v.py`: explores plausible return contracts for the original **30-byte** `AsmOutput_WritePrologue`, without an assembly stub. The tested ostream-reference candidate remains **22/30**.
- `evaluate_asm_prologue_source_forms_oct15v.py`: captures a bounded genuine C++03 source and historical compiler-flag search for the same 30-byte prologue. Preliminary baseline `-O2` remains **22/30**. Earlier extensive source/flag controls are documented separately; the evaluator is retained as a reproducible continuation checkpoint, not a claim of novelty.

These experimental scripts do not change production code or the acceptance manifest. A partial mismatch is not an exact match; preserve negative experiments and commit productive intermediate C++03 source frequently so that another session can build on them. Only an original-length function with all relocation and literal ownership authenticated may enter the selected EXPERIMENTAL queue; only **every instruction byte and every required relocation** matching may enter the EXACT queue and `vuc/src/`. Keep the original binaries and unrelated untracked files untouched.

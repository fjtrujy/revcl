# Instruction arena chunk-owner source forms: bounded negative control

Date: 2026-10-16. VUC 2004.1 Linux i386, original callback `__tcf_1` at `0x08051980` (46 instruction bytes).

The interrupted session left `vuc/probes/evaluate_instruction_chunk_delete_oct15v.py` as an untracked but complete source-level evaluator. Preserve it as a reproducible negative control; no generated instruction bytes, inline assembly, relocation patching, or alignment padding is used.

The evaluator generates 20 C++03 source forms: four authentic candidate ownership declarations (`char*`, `unsigned char*`, `InstructionChunk*` holding `char[0x2004]`, and `InstructionChunk*` holding `unsigned char[0x2004]`) crossed with five cleanup lifetimes (`delete`, explicit global operator delete, reference to the global head, `for`, and `do/while`). Each accesses the original `+0x2004` linked-list pointer through a concrete source-level member or pointer expression. The cleanup executes as the destructor of a genuine global static object.

Reproduction inside the existing `revcl-oracle` guest:

```sh
python3 /path/to/revcl/vuc/probes/evaluate_instruction_chunk_delete_oct15v.py \
  > /tmp/evaluate_instruction_chunk_delete_oct15v.py.log 2>&1
```

The historical Jan25 GCC 3.3.3 prerelease wrapper `/tmp/vuc-binutils215/g++-jan25-as215`, with `-O2 -frename-registers -fuse-cxa-atexit`, produced original-sized 46-byte candidates for **all 20 variants**. The strict relocation-aware `match_probe.py` comparison returned **45/46 matching original-offset instruction bytes** for every variant, with the sole mismatch at relative byte offset **+42**. No variant was byte-exact. The original `pop %ecx` and generated `pop %eax` differ in a dead epilogue register choice. This result is consistent with, rather than an improvement over, the previously selected experimental cleanup candidate.

The result eliminates this bounded family of arena-pointee type and straightforward cleanup-lifetime hypotheses; it does not prove the original ownership type or explain the compiler register choice. A future experiment needs a different discriminator, such as genuine preceding translation-unit definitions or RTL lifetime evidence, not another spelling of these loops. Do not promote the candidate to `vuc/src/` or change `exact_candidates.json` on this evidence alone.

# VUC 2004.1 — Recover first 166-byte hash-table copy-constructor partial

Date: October 14, 2026. Repository: `/path/to/revcl`.
Branch: `vuc`. Target: original VUC 2004.1 ELF32 i386 from the
unmodified tracked upstream archive.

## Git and preservation

The initial branch was `vuc` at commit `fa88087`, with **163
pre-existing, unrelated untracked paths**. Executed `git fetch origin
&& git rebase origin/main`; `origin/main` was already an ancestor
and no additional commits were rebased. No new worktrees or subagents
were used. All original VUC inputs, root VCL sources and unrelated
untracked paths were preserved.

## Newly registered experimental function

The original
`MLHashTable<MLGameString, SymbolTableEntry*, MLHashFun<MLGameString>,
513>::MLHashTable(const MLHashTable&)` at **0x0804f420** was
previously **UNRESOLVED**, original size **166** bytes. Its current
natural `new Node(*node)` source in
`vuc/probes/hash_copy_constructor.cpp` emits 162 bytes, cannot
qualify as an original-length partial, and therefore remains intact.

This batch recovered a genuine, semantically meaningful
original-length implementation:

- Source: `vuc/probes/hash_copy_node_args_partial_oct14.cpp`.
- Provisional **probe-local** Node class declaration:
  `vuc/probes/MLHashTableCopyNodePartial.h`. The previously
  independently accepted `vuc/src/MLHashTable.h` was not altered.
- Selected manifest:
  `vuc/config/hash_copy_node_args_partial_oct14_probe.json`.
- Original size and compiled size: **166/166** bytes.
- **80/166 original-offset identical instruction bytes**, original
  contiguous entry prefix **8 bytes**.
- All **4/4 original relocation sites** authenticated:
  `operator new(unsigned)` at `0x080499ec`,
  `MLGameString` copy constructor at `0x08071770`,
  `operator delete(void*)` at `0x0804973c`,
  and `_Unwind_Resume` at `0x0804998c`.
- Compiler: original-era **GCC 3.3.3** with GNU as **2.15**,
  `-O2 -frename-registers -falign-jumps=1`.
- Independently compiled and compared under *both* recovered compiler
  snapshots: January 25, 2004, and December 6, 2003. Both give
  `length=166, exact_bytes=80, relocations=4, exact=false`.

### Original semantics recovered

The source iterates 513 hash buckets, resets the output heads,
iterates each original singly linked list, captures the next
source node before copying, allocates a new node with four real
field values, and appends the new copy to the destination tail.

The key difference from the earlier model is a *plausible C++03
explicit Node constructor*, supplied with original key/hash/value/next
fields, rather than `new Node(*node)`. This causes GCC to schedule
the field copies differently and yields correct total original
function length without patched instructions, arbitrary padding,
fake function addresses or forced register assignments.

The exact historical `Node` constructor prototype is **not proved**
by this experiment. The original ELF places the bucket index at
`[ebp-0x10]`, while the current provisional implementation
uses a register-carried index. The original also hoists different
field loads around the `MLGameString` copy call. These explain
why the candidate is meaningfully partial but *not* byte-exact.

## Reproducible probes and bounded negative controls

| Driver | Compiles | Outcome |
| --- | ---: | --- |
| `evaluate_value_logical_real_source_units_oct14.py` | 84 | Real comparison / logical function source-unit arrangements preserve exact `Value::operator||` at 309/309; `Value::operator&&` remains 190/325 original-length |
| `evaluate_value_tostring_late_align_oct14.py` | 50 | Original-length `Value::ToString` best 159/221, all 8 call/literal relocations and 82 original literal bytes verified |
| `evaluate_value_tostring_switch_order_oct14.py` | 42 | Source-level lexical permutations of three genuine format cases preserve the same 159/221 best. An initial scratch-only relative include problem was corrected before final scores |
| `evaluate_value_and_operand_predictions_oct14.py` | 64 | Genuine C++03 vector lane predicate controls: original-length logical AND best remains 190/325; wrong-sized 357-/373-byte bodies overlap more offsets but do not qualify |
| `evaluate_hashcopy_node_ctor_owner_oct14.py` | 144 | Tested implicit / explicit user-defined Node copy and four-argument constructors, six true copy/advance topologies, six historical compiler alignment configurations; discovered 166-byte, 80/166 candidate |
| `evaluate_hashcopy_node_argument_order_oct14.py` | 72 | All 24 valid constructor argument orders × three alignment modes. Original-length best remains 80/166. Alternative parameter orderings do not recover stack-index liveness |

**456** GCC source/flag recipes in this batch; only the original
hash-table constructor advances from unresolved into a validated
original-length **EXPERIMENTAL** reconstruction. All compiler objects,
generated variants, and complete `results.json` records are under
the scripts' individual `/tmp/vuc-binutils215` paths in the existing
`revcl-oracle` Lima guest, not staged or claimed as original data.

The exact-function manifest does not include the new partial.
Previously exact `Value::operator||` and the original-length 190/325
`Value::operator&&` candidate remain unchanged.

## Reproduce independently

```sh
R=/path/to/revcl/vuc
for era in jan25 dec06; do
  limactl shell revcl-oracle -- \
    /tmp/vuc-binutils215/g++-${era}-as215 \
    -O2 -frename-registers -falign-jumps=1 \
    -c "$R/probes/hash_copy_node_args_partial_oct14.cpp" \
    -o "/tmp/vuc-binutils215/hash-copy-args-${era}-verify.o"
  limactl shell revcl-oracle -- python3 "$R/scripts/match_probe.py" \
    --object "/tmp/vuc-binutils215/hash-copy-args-${era}-verify.o" \
    --config "$R/config/hash_copy_node_args_partial_oct14_probe.json"
done
```

For exact accepted functions (including every original relocation),
initialized data, BSS and Python regression tests, run the normal
`vuc/scripts/match_candidates.py`,
`vuc/scripts/match_data_candidates.py`,
`vuc/scripts/match_bss_candidates.py` and
`python3 -m unittest discover -s vuc/tests -q`.

The expected ledger advances from **200 EXACT / 23 EXPERIMENTAL /
75 UNRESOLVED** to **200 EXACT / 24 EXPERIMENTAL /
74 UNRESOLVED** out of 298 original named functions.
The exact instruction-byte corpus remains **14,930**.
This does **not** establish whole-linked-ELF byte identity.

Next follow-up: reconstruct the exact source type/ownership of the
stack-resident outer bucket index and genuine Node constructor
field-copy evaluation order; avoid another broad undirected jump-
alignment sweep and don't promote a wrong-sized candidate.

# Additional independent VUC function probes

Historical compiler: GCC 3.3.3 (20040125) with GNU as 2.15 in
`revcl-oracle`. Original ELF bytes and resolved relocations are the
only acceptance authority. These are negative results, not promotions.

| Target | Reproduction | Best result | Observation |
| --- | --- | --- | --- |
| `InstructionList::GenLabel` | `evaluate_genlabel.py` | 34/85 | Source ordering and temporary names do not reproduce original |
| `MLGameStringTable::Get` | `evaluate_get_volatile_flags.py` | 102/106 | Tested 19 optimization variants; none exact |
| `yyFlexLexer::yy_delete_buffer` | `evaluate_yy_delete.py` | 85/87 | Different flag and ownership expressions retain the same mismatch |

The `Value` bitwise operator probe (`evaluate_value_bitwise_order.py`)
also reports **143/143** for both `operator&` and `operator|` when
`result` is declared before `temporary` under `-O2
-frename-registers`. This is a **regression confirmation**, not a
new discovery: both functions were already included in
`vuc/config/exact_candidates.json` before this investigation.
Their existing accepted source in `vuc/src/ValueBitwise.cpp` already
uses the correct declaration order. Do not double-count them.

Next investigations should target other unresolved original functions,
not repeat these flag sweeps without new source or toolchain evidence.

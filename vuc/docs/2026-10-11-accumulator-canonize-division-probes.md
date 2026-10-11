# Additional original-ELF reconstruction experiments

These are negative results under the historical January 2004 GCC 3.3.3
and GNU as 2.15 toolchain. No candidate is accepted without exact
code bytes and authenticated relocation targets.

| Target | Probe | Best observed score | Outcome |
| --- | --- | --- | --- |
| `IsAccumulatorable` | `evaluate_accumulatorable.py` | 61/77 | 11 control-flow shapes and four compiler options; no exact result |
| `Canonize2_Reduce` field type | `evaluate_canonize_field.py` | 83/85 | Ten signed/unsigned, volatile, enum and bitfield representations converge to the same result |
| `Value::operator/` | `evaluate_value_division_order.py` | 174/235 | Six switch-case permutations match identically under jump alignment 8; no exact result |

`evaluate_ast_arena_init.py` additionally reaches 138/138 for the
AST arena initializer using the cursor-first or typed-constructor
source variant. **This is not a new match**: the corresponding
`ast_arena_initializer_probe.json` was already included in the exact
candidate manifest. Its reproduction is useful as a regression check,
but it must not increase the accepted-function count.

The field-type and switch-order experiments rule out more equivalent
source permutations as a productive next step. Prioritize previously
unreconstructed functions and original DWARF/compiler evidence.

# Multi-function byte-exact search: additional negative evidence

All comparisons use the original ELF and the recovered January 2004
GCC 3.3.3 / GNU as 2.15 toolchain. Existing exact functions are never
counted as new discoveries.

| Target | Probe | Result |
| --- | --- | --- |
| `Check(ASTFunCall*)` | `evaluate_check_fun_copy.py` | 118/118 with implicit copy or `memcpy`; already in exact manifest |
| `MLTempString::Set` | `evaluate_ml_temp_set_constructor.py`, `evaluate_ml_temp_set_flags.py` | No exact match; constructor variants compiled at 118 bytes instead of original 117 |
| `yyFlexLexer::yy_load_buffer_state` | `evaluate_yy_load.py` | 24/39 at best |
| `yyFlexLexer::yy_create_buffer` | `evaluate_yy_create.py` | 131/139 at best |
| `_GLOBAL__I_inputFileName` | `evaluate_iverse_ostream_flags.py` | 29/31; 38 flag combinations, none exact |
| `Print(ASTFunCall*)` | `evaluate_print_fun_call.py` | 92/98 in this source-variant sweep |

For the 31-byte startup wrapper, the original GCC-generated wrapper
uses EDX for the `0xffff` argument, while the reconstructed translation
unit uses EAX. The difference affects two instruction bytes, not the
relocated call target. None of the 38 compiler-option combinations
recovered the original register choice. Future investigation should
consider translation-unit context rather than repeat flag sweeps.

`evaluate_is_foldable.py` was also exercised; `IsFoldable` is already
in the exact manifest, so it is not eligible for a new-function count.

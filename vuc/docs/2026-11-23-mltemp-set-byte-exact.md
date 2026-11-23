# VUC 2004.1 — MLTempString::Set is byte-exact

2026-11-23, branch vuc. Original VUC 2004.1 Linux ELF32, address 0x08071d00.

The original MLTempString::Set(const char*, ...) is 117 instruction bytes.
Previously 111/117 bytes matched at original offsets; the six mismatches
were +32,+42,+71,+76,+82,+86. The new C++03 source matches **117/117**
and resolves all **8/8** original call and singleton-data relocations.
Both GCC 3.3.3 historical Jan25 and Dec06 snapshots produce the same result.

## Why the missing registers matched

Adding the real MLTempString::operator+= definition as a predecessor
changed the singleton/memset scratch registers and increased overlap to
115/117. The last two byte differences were the format-pointer load and
argument store at offsets +32 and +42. Compiling -O2 -frename-registers
with this genuine method definition order makes all 117 bytes exact:

1. MLTempString::operator+=(const MLTempString&)
2. MLTempString default constructor
3. MLTempString::Set(const char*, ...)
4. MLTempString formatted constructor

Source: vuc/src/MLTempStringSetExactOwner.cpp.
Exact manifest: vuc/config/ml_temp_set_probe.json.
Experiment: vuc/probes/evaluate_mltemp_set_real_neighbor_owner_20261123.py.

No inline assembly, register binding, dummy dependency, padding,
manual instruction editing, or modifications to the vendor ELF were used.
The return type MLTempString* remains a source hypothesis: the ABI
symbol does not encode its return type, but the original returns this in EAX.
The existing provisional native header declares void Set. This new
historical owner is deliberately independent until the shared class
declaration is recovered.

**Function-level acceptance only**: the preceding operator+= in this
object has different literal provenance, and the two emitted constructors
are not all exact from this owner. The accepted owner is match-only and
is not included in the native VUC object target or linked to the other
historical owner files.

## Post-promotion coverage

- 210/298 exact functions, 17,051/17,051 exact-function bytes.
- 34 experimental functions; 33 eligible original-length partials:
  6,879/10,806 original-offset bytes within those partial functions.
- 54 unresolved functions; one experimental SymbolTable::Enter is
  wrong-sized and not scored.
- Original data, BSS, whole VUC executable remain distinct gates.
- A fully linked byte-identical VUC executable is not yet available.

See FUNCTION_BYTE_MATCH_STATUS.md for the latest generated inventory.

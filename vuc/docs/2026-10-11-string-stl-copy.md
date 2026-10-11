# VUC historical string STL copy — byte-exact checkpoint

Date: October 11, 2026. Target: the original VUC 2004.1 i386 ELF.

## Source reconstruction

The previously unresolved
`std::__uninitialized_copy_aux<__gnu_cxx::__normal_iterator<std::string*,std::vector<std::string>>, ...>`
at **0x0805b2e0** is now **161/161 bytes exact**.

The accepted `vuc/src/StringUninitializedCopy.cpp` contains a normal
explicit instantiation of the historical GCC 3.3
`<bits/stl_uninitialized.h>` template over
`std::vector<std::string>::iterator`. The historical GCC 3.3.3
20040125 source-date candidate and GNU as 2.15 produce the original
code without inline assembly, padding, binary blobs or special-case
linker offsets.

The verifier independently resolves these six original linked targets:
`std::string` copy constructor at `0x080498bc`,
`__cxa_begin_catch` at `0x0804995c`,
`std::string::_Rep::_M_destroy` at `0x0804991c`,
`__cxa_rethrow` at `0x0804979c`,
`__cxa_end_catch` at `0x0804983c`, and
`_Unwind_Resume` at `0x0804998c`.

This reconstructs one specific emitted template instantiation, not
the complete historical `ListFile.cpp` compilation unit.

## Aggregate strict gate

The strict function gate now reports **147/147 exact named original
function records, 8,325/8,325 code bytes, 370 resolved relocations,
132 verified literal bytes**. Original inventory:
**147 EXACT, 10 EXPERIMENTAL, 141 UNRESOLVED** out of 298.

Reproduce:

```sh
python3 -m unittest discover -s vuc/tests -q
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/exact-gate-147
```

This is function-scoped acceptance only. The **mandatory full ELF
byte-for-byte match remains unachieved**; no whole-binary percentage
is implied by the figures above.

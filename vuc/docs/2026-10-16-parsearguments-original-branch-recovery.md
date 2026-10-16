# ParseArguments: restore ELF-observed filename branches before byte matching

Date: 2026-10-16. Original VUC 2004.1 Linux i386 function `_Z14ParseArgumentsiPPc` at `0x0805b3e0`, **777 instruction bytes**.

The existing behavioral draft `vuc/probes/ParseArgumentsFreshOct15t.cpp` compiled to 681 or 697 bytes under historical GCC 3.3.3, depending on the extension-append source idiom. The original ELF disassembly exposes three additional source-level facts missing or inverted in that draft:

1. At `0x0805b4aa` the explicit output-argument path calls `strlen` on `argv[options+2]` and rejects lengths above 255 before copying to `outputFileName`. The earlier draft did not check the supplied output filename length.
2. The generated-output path at `0x0805b580` copies `argv[options+1]` into `outputFileName`, replaces/appends `.vcl` using `strrchr('.')`, then compares the completed output name with `inputFileName`. The old source performed suffix handling in the explicit-output branch instead.
3. The explicit-output path at `0x0805b506` searches `inputFileName` for slash/backslash and copies the directory prefix to `inputPath`, returning success. The original's variable-length copy at `0x0805b51f` uses `cld`, `rep movsl`, conditional 2-byte/1-byte tails and a trailing NUL store. The old source called `memcpy` only after both output branches, which does not reproduce this control flow.

`vuc/probes/ParseArgumentsOriginalBranchesOct16.cpp` retains ordinary C++03 fixed-buffer and error-reporting source, but recovers those original branch relationships. It preserves the historical machine-code `options+3 < argc` check even though its indexing may reflect an original off-by-one argument-handling bug; silently correcting that check would be speculation. The original archive is unchanged.

Historical compilation (separately verified with the Jan25 and Dec06 GCC 3.3.3/binutils 2.15 wrappers) gives `_Z14ParseArgumentsiPPc` **0x2c9 = 713 bytes**, not 777, under `-O2 -frename-registers`. `-O3 -frename-registers` also yields 713. The alternative `-minline-all-stringops` emits 0x439 = 1081 bytes at default jump alignment and 0x420 = 1056 with `-falign-jumps=1`; those variants still fail original-length eligibility. The unmodified normal source still calls external `memcpy` while the original emits inlined string-copy instructions. This is a new behavioral/source-control-flow recovery, **not a valid 713/777 strict partial score**, and no candidate was added to `selected_candidates.json` or `exact_candidates.json`.

To reproduce inside `revcl-oracle`:

```sh
R=/path/to/revcl/vuc
/tmp/vuc-binutils215/g++-jan25-as215 -O2 -frename-registers \
  -c "$R/probes/ParseArgumentsOriginalBranchesOct16.cpp" \
  -o /tmp/vuc-parse-original-branches-oct16.o
nm -S /tmp/vuc-parse-original-branches-oct16.o | grep _Z14ParseArgumentsiPPc
```

Next discriminator: recover the original source construct that makes historical GCC emit a variable-length `rep movsl` plus 2/1-byte tails for the `inputPath` copy **without** inlining all the other string operations in the translation unit. An honest original-length object and independently authenticated relocations/literals are prerequisites for selecting a partial.

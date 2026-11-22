# Third-party notices and provenance

REVCL is an independent reconstruction project. It is not affiliated with,
sponsored by, or endorsed by Sony or the original VUC developers.

## Sony VCL 1.4 beta 7

The target is Sony Computer Entertainment America's historical VU Command Line
(VCL) tool. The original executable, user manual, original binary disassembly,
and vendor release archives are **not included** in this repository.

The original VCL 1.4 beta 7 x86 release contains a Sony `LICENSE` permitting
copying and redistribution of its **unmodified software and documentation**
when the license accompanies them. That grant is specific to the original
release materials. It must not be interpreted as Sony licensing the
reconstructed C++ sources here, or relicensing Sony's materials under MIT.

The source in `src/` aims to reproduce the original executable when compiled
with a period-correct compiler. This technical relationship does not itself
resolve copyright questions about redistribution of the reconstruction.

Historical project reference:
https://ps2linux.no-ip.info/playstation2-linux.com/projects/vcl.html

## VUC 2004.1

Work on the distinct VUC compiler is maintained on the `vuc` branch. Original
VUC executables and archives are not included in either branch. The historical
VUC distribution has its **own terms**, including restrictions on commercial
use and redistribution of modified compiler versions. Those terms are not the
Sony VCL license or the REVCL MIT license. Consult the original VUC license
before obtaining, using, or redistributing that material.

## Historical development toolchain

The optional historical build scripts retrieve externally maintained Red Hat
Linux 9 compiler/runtime packages. Those downloads are not committed here
and retain their respective upstream licenses. Their presence in a local
build environment does not make them part of REVCL's MIT-licensed sources.

## Scope of the repository license

`LICENSE` specifies the terms intended for contributions to REVCL **to the
extent that their copyright holders have authorized those terms**. It cannot
grant rights to Sony's original software, the VUC authors' software, or any
other material owned by third parties. Commit attribution alone does not
establish a contributor's copyright ownership or permission to relicense.

Please consult [CONTRIBUTING.md](CONTRIBUTING.md) before submitting source or
research material. Rights-clearance questions should be resolved before public
redistribution, independently of whether the build is byte-identical.

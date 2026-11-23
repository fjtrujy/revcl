# Contributing to REVCL

Thank you for helping reconstruct and test historical PlayStation 2 tools.
REVCL contains the byte-exact VCL reconstruction in `src/` and the separate,
ongoing VUC reconstruction in `vuc/` under a shared CMake project.

## Source provenance and licensing

- Submit only code, tests, and text you created or are authorized to share
  under the repository's intended MIT terms. If the provenance or licensing
  of a contribution is unclear, discuss it before submitting.
- Do **not** commit original vendor executables, archives, manuals,
  disassemblies, proprietary source code, confidential SDK materials,
  unlicensed third-party code, or bulk copies of original machine code.
- Do not upload private development configuration, local paths, credentials,
  personally identifying information about others, or unreviewed research
  dumps. Use locally downloaded reference inputs and keep them out of Git.
- Attribute existing authors and preserve any third-party notices. A
  contribution must not imply that MIT grants rights in Sony or VUC software.
- Git commit authorship is a record of changes, not proof of copyright
  ownership or relicensing consent. If you submit someone else's work, obtain
  permission first.

## VCL development (`main`)

Use the README's CMake presets and tests. For changes to the reconstructed
source, verify modern native compiler checks and the historical byte-exact
SHA-256 target whenever the historical environment is available. Describe
whether a change preserves the exact binary or changes only modern behavior.

## VUC development (`vuc/`)

Follow `vuc/README.md` and its existing research/test workflows. Original
VUC ZIPs must be obtained separately and verified against the recorded
checksums. Distinguish exact function matches from partial candidates in
reports; don't describe the whole VUC binary as matched until verified.

## Pull requests

Keep changes focused, explain the evidence or bug addressed, include
reproduction/test commands, and identify any new external dependencies.
Document the source and licensing of new third-party material before it is
included. Do not submit artifacts or data from a source whose distribution
terms are uncertain.

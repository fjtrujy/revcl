#!/usr/bin/env python3
"""Forensic only: source-authenticated exact common prefix of the nonexact
VUC __uninitialized_copy_aux<Scope*> implementation. Do not treat a shorter
candidate as an exact/selected function or change matching acceptance rules.
"""
import importlib.util,json,subprocess
from pathlib import Path
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('match',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=json.loads((R/'vuc/config/scope_copy_prefix_diagnostic_oct14_probe.json').read_text())
obj=Path('/tmp/vuc-binutils215/scope-prefix-119byte-oct14.o')
flags=cfg['flags']
cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*flags,
 '-c',str(R/cfg['source']),'-o',str(obj)],
 capture_output=True,text=True)
if cp.returncode:raise RuntimeError(cp.stderr)
orig_binary,target=m.decomp.load()
sec=next(x for x in target['sections'] if x['name']=='.text')
address=int(cfg['address'],0)
start=sec['offset']+address-sec['address']
original=orig_binary[start:start+cfg['size']]
normalized,relocations=m.prepare_named_function(obj.read_bytes(),cfg['function'],
 cfg['relocations'],address)
prefix=next((i for i,(x,y) in enumerate(zip(normalized,original)) if x!=y),
 min(len(normalized),len(original)))
matches=sum(x==y for x,y in zip(normalized,original))
result={'target':cfg['function'],'original_address':cfg['address'],
 'original_length':len(original),'candidate_length':len(normalized),
 'relocations_verified':relocations,'exact_common_prefix':prefix,
 'identical_overlapping_offsets':matches,'scored_as_exact':False,
 'source':cfg['source'],'flags':flags}
print(json.dumps(result,indent=2),flush=True)
assert relocations==6
assert len(normalized)==119 and len(original)==120
assert prefix==75, 'historical copy-loop prefix unexpectedly changed'
assert matches>=84

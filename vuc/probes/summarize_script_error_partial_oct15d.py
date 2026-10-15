#!/usr/bin/env python3
"""Summarize completed historical-GCC script_error experimental objects.

The optional iostream source-form matrix takes considerable time in the
recovered GCC 3.3.3 VM. Preserve an auditable, relocation-validated result
from objects *already compiled* if a bounded batch is intentionally ended.
This never promotes a wrong-sized/unsupported-relocation candidate.
"""
from pathlib import Path
import importlib.util,json,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('vuc_match_script_error',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
source=R/'vuc/config/script_error_probe.json'
T=Path('/tmp/vuc-binutils215/vuc-script-error-lifetimes-oct15d')
items=[];counts=collections.Counter()
objects=sorted(x for x in T.glob('*.o') if x.name!='best.o')
for o in objects:
 try:result=m.run(o,source)
 except ValueError as e:
  items.append(dict(tag=o.stem,error=str(e)[:160]));counts['invalid']+=1;continue
 row=dict(tag=o.stem,length=result['length'],match=result['exact_bytes'],relocations=result['relocations'],literal_bytes=result['verified_literal_bytes'],exact=result['exact'],different_offsets=result['different_offsets'])
 if row['length']!=161 or row['relocations']!=15 or row['literal_bytes']!=5:
  row['ineligible']=True;counts['ineligible']+=1
 else: counts['valid']+=1
 items.append(row)
best=sorted((x for x in items if 'match' in x and not x.get('ineligible')),key=lambda x:x['match'],reverse=True)
doc=dict(compiled_object_count=len(objects),validation_counts=dict(counts),best=best[:12],results=items)
(T/'partial_results.json').write_text(json.dumps(doc,indent=2)+'\n')
print(json.dumps({k:v for k,v in doc.items() if k!='results'},indent=2))

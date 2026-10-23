#!/usr/bin/env python3
"""Recover original literal ownership for compiler-reordered ConcatenateBinOp strings.

For each trial object, map every emitted NUL-terminated .rodata item to its
UNIQUE existing byte-identical original ELF address from the authenticated
selected ConcatenateBinOp config. Reject missing/extra/duplicate strings and
all unknown original targets. Then run the unchanged strict relocation/byte
comparator; never map literals by an invented address.
"""
from pathlib import Path
import importlib.util,json,collections
R=Path(__file__).resolve().parents[1]
T=Path('/tmp/vuc-concat-opcode-prediction-oct23')
sp=importlib.util.spec_from_file_location('matcher',R/'scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
baseline=json.loads((R/'config/concatenate_binop_partial_oct15o.json').read_text())
original,info=m.decomp.load()
section=next(x for x in info['sections'] if x['name']=='.rodata')
expected={}
for item in baseline['relocation_literals']['.rodata']:
 offset=section['offset']+int(item['address'],0)-section['address']
 b=original[offset:offset+item['size']]
 assert b and b[-1:]==b'\0' and b not in expected,b
 expected[b]=item['address']
assert len(expected)==16
rows=[]
for stem in ('op'+str(k)+'-prediction'+str(pred) for k in (13,12,11,10,9,8,7,6,5,4,3,2,1,0) for pred in (0,1)):
 record={'case':stem}
 for snapshot in ('jan25','dec06'):
  obj=T/(stem+'-'+snapshot+'.o')
  if not obj.exists():record[snapshot]={'error':'object missing'};continue
  raw=obj.read_bytes()
  secs,_=m.elf_sections(raw)
  if '.rodata' not in secs:record[snapshot]={'error':'no .rodata'};continue
  data=m.section_bytes(raw,secs['.rodata'][1])
  index=0; ranges=[];seen=[]
  try:
   while index<len(data):
    end=data.find(b'\0',index)
    if end<0:raise ValueError('unterminated string')
    payload=data[index:end+1]
    if payload not in expected:raise ValueError('unauthenticated literal '+repr(payload))
    ranges.append({'offset':index,'size':len(payload),'address':expected[payload]})
    seen.append(payload)
    index=end+1
   if collections.Counter(seen)!=collections.Counter(expected.keys()):
    raise ValueError('missing or extra original literal')
   cfg=dict(baseline,relocation_literals={'.rodata':ranges})
   cfgfile=T/(stem+'-'+snapshot+'-validated-literals.json')
   cfgfile.write_text(json.dumps(cfg,indent=2)+'\n')
   score=m.run(obj,cfgfile)
   record[snapshot]={'size':score['length'],'exact_bytes':score['exact_bytes'],
    'relocations':score['relocations'],'verified_literals':score['verified_literal_bytes'],
    'mismatches':score['different_offsets'][:16],'config':str(cfgfile)}
   if score['exact_bytes']>223:print('IMPROVED',stem,snapshot,score['exact_bytes'],flush=True)
  except (ValueError,KeyError) as error:record[snapshot]={'error':str(error)}
 rows.append(record)
 print(stem,record['jan25'].get('exact_bytes',record['jan25'].get('error')),flush=True)
(T/'literal-reorder-results.json').write_text(json.dumps(rows,indent=2)+'\n')
print('BEST',sorted([(x['jan25']['exact_bytes'],x['case']) for x in rows
 if 'exact_bytes' in x['jan25']],reverse=True)[:12],flush=True)

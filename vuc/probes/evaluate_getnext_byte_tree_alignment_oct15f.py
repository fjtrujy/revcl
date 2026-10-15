#!/usr/bin/env python3
"""VUC GetNextWord natural byte-range comparison tree + GCC label/loop alignment.

The original target compares signed 8-bit DL to 44,9 and branches between
natural low/high delimiter ranges. Previous syntactically equivalent
range-ordered C++03 source is 143/151 bytes under common alignments.
Test GCC3.3.3 target-generated jump/label/loop placement to recover
original 135 bytes. Never add filler or patch instructions.
"""
from pathlib import Path
import json,itertools,subprocess,importlib.util,collections
R=Path(__file__).resolve().parents[2];s=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
cfg=R/'vuc/config/get_next_word_partial_oct15e_probe.json'
baseline=(R/'vuc/probes/GetNextWordRange36BaselineOct15f.cpp').read_text()
assert 'char c=*input;' in baseline
assert 'if(c>44)' in baseline
leading='if (c <= 10) { if (c < 9) break; }\n else if (c != 32) break;'
assert leading in baseline
sources={
 'char':baseline,
 'signed':baseline.replace('char c=*input;', 'signed char c=*input;'),
 'unsigned':baseline.replace('char c=*input;', 'unsigned char c=*input;'),
 'char-inverted':baseline.replace(leading,
                     'if(c>10){if(c!=32)break;} else if(c<9)break;'),
}
flags={
 'j1l1':['-O2','-frename-registers','-falign-jumps=1','-falign-labels=1'],
 'j2l1':['-O2','-frename-registers','-falign-jumps=2','-falign-labels=1'],
 'j4l1':['-O2','-frename-registers','-falign-jumps=4','-falign-labels=1'],
 'j8l1':['-O2','-frename-registers','-falign-jumps=8','-falign-labels=1'],
 'j16l1':['-O2','-frename-registers','-falign-jumps=16','-falign-labels=1'],
 'j1l2':['-O2','-frename-registers','-falign-jumps=1','-falign-labels=2'],
 'j2l2':['-O2','-frename-registers','-falign-jumps=2','-falign-labels=2'],
 'j4l2':['-O2','-frename-registers','-falign-jumps=4','-falign-labels=2'],
 'j8l2':['-O2','-frename-registers','-falign-jumps=8','-falign-labels=2'],
 'j16l2':['-O2','-frename-registers','-falign-jumps=16','-falign-labels=2'],
 'j1l4':['-O2','-frename-registers','-falign-jumps=1','-falign-labels=4'],
 'j2l4':['-O2','-frename-registers','-falign-jumps=2','-falign-labels=4'],
 'j4l4':['-O2','-frename-registers','-falign-jumps=4','-falign-labels=4'],
 'j8l4':['-O2','-frename-registers','-falign-jumps=8','-falign-labels=4'],
 'j16l4':['-O2','-frename-registers','-falign-jumps=16','-falign-labels=4'],
 'j1l8':['-O2','-frename-registers','-falign-jumps=1','-falign-labels=8'],
 'j2l8':['-O2','-frename-registers','-falign-jumps=2','-falign-labels=8'],
 'j4l8':['-O2','-frename-registers','-falign-jumps=4','-falign-labels=8'],
 'j8l8':['-O2','-frename-registers','-falign-jumps=8','-falign-labels=8'],
 'j16l8':['-O2','-frename-registers','-falign-jumps=16','-falign-labels=8'],
}
T=Path('/tmp/vuc-binutils215/getnext-byte-tree-alignment-oct15f');T.mkdir(exist_ok=True,parents=True)
rows=[];sizes=collections.Counter();best=None
for (name,source),(fl,base),loop in itertools.product(sources.items(),flags.items(),('1','2','4','8')):
 tag=f'{name}-{fl}-loop{loop}'
 opts=base+['-falign-loops='+loop]
 cpp=T/(tag+'.cpp');obj=T/(tag+'.o');cpp.write_text(source)
 p=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if p.returncode:rows.append(dict(tag=tag,error=p.stderr[-120:]));continue
 try:d=m.run(obj,cfg)
 except ValueError as e:rows.append(dict(tag=tag,error=str(e)[:130]));continue
 row=dict(tag=tag,length=d['length'],match=d['exact_bytes'],relocations=d['relocations'],prefix=next(iter(d['different_offsets']),d['length']))
 rows.append(row);sizes[d['length']]+=1
 if row['length']==135 and row['relocations']==5 and (best is None or (row['match'],row['prefix'])>(best['match'],best['prefix'])):
  best=row;(T/'best.cpp').write_text(source);(T/'best.o').write_bytes(obj.read_bytes());(T/'best.flags').write_text(' '.join(opts)+'\n')
  if row['match']>25:print('IMPROVED',json.dumps(row),flush=True)
  if row['match']==135:print('EXACT',json.dumps(row),flush=True);break
(T/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
print(json.dumps(dict(attempted=len(rows),sizes=dict(sizes),strict=sum(r.get('length')==135 and r.get('relocations')==5 for r in rows),best=best,top=sorted((x for x in rows if 'match' in x),key=lambda r:(r['match'],r['prefix']),reverse=True)[:10]),indent=2))

#!/usr/bin/env python3
"""Historical GCC3.3.3 pass transfer across already-selected nonexact VUC sources.
Rerun genuine source candidates with a single independent compiler pass toggle.
Only real original-sized, named-relocation- and literal-checked gains qualify
as experimental improvements; no source modification or synthetic padding.
"""
from pathlib import Path
import itertools,importlib.util,subprocess,json,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
sel=json.loads((R/'vuc/config/selected_candidates.json').read_text())
exact=set(json.loads((R/'vuc/config/exact_candidates.json').read_text()))
sources=[c for c in sel if c not in exact]
flags={
 'baseline':[],
 'no-crossjump':['-fno-crossjumping'],
 'no-reorder':['-fno-reorder-blocks'],
 'no-sched2':['-fno-schedule-insns2'],
 'no-regmove':['-fno-regmove'],
 'no-gcse':['-fno-gcse'],
 'no-guess':['-fno-guess-branch-probability'],
 'no-thread':['-fno-thread-jumps'],
 'no-csefollow':['-fno-cse-follow-jumps'],
 'j1':['-falign-jumps=1'],
 'j4':['-falign-jumps=4'],
 'j8':['-falign-jumps=8'],
 'rename':['-frename-registers'],
}
T=Path('/tmp/vuc-binutils215/vuc-experimental-pass-transfer-oct15c');T.mkdir(parents=True,exist_ok=True)
rows=[];improved=[];sizes=collections.Counter()
for index,cpath in enumerate(sources):
 config=json.loads((R/cpath).read_text());source=R/config['source']
 candidate_scores=[]
 for name,extra in flags.items():
  tag=f'{index:02d}-{name}';objectfile=T/(tag+'.o');manifest=T/(tag+'.json')
  opts=[*config.get('flags',[]),*extra];manifest.write_text(json.dumps(dict(config,flags=opts))+'\n')
  compiled=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(source),'-o',str(objectfile)],capture_output=True,text=True)
  if compiled.returncode:
   candidate_scores.append(dict(mode=name,error=compiled.stderr[-130:]));continue
  try:
   result=m.run(objectfile,manifest)
  except ValueError as e:
   candidate_scores.append(dict(mode=name,error=str(e)[:140]));continue
  row=dict(mode=name,length=result['length'],match=result['exact_bytes'],relocations=result['relocations'],prefix=next(iter(result['different_offsets']),result['length']),literal_bytes=result.get('verified_literal_bytes',0),exact=result['exact'])
  candidate_scores.append(row)
 base=next((x for x in candidate_scores if x.get('mode')=='baseline'),{})
 best=sorted((x for x in candidate_scores if 'match' in x and x['length']==config['size']),key=lambda x:(x['match'],x['prefix']),reverse=True)
 rec=dict(config=cpath,function=config['function'],address=config['address'],baseline=base,best=best[:4],count_compiled=sum('match' in x for x in candidate_scores))
 rows.append(rec)
 if best and 'match' in base and best[0]['match']>base['match']:
  improved.append(rec)
 print('CHECK',index+1,len(sources),config['address'],'old',base.get('match'),'new',best[0]['match'] if best else None,'mode',best[0]['mode'] if best else None,flush=True)
(T/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
print(json.dumps(dict(attempted=len(sources)*len(flags),source_units=len(sources),new_gains=improved),indent=2))

#!/usr/bin/env python3
"""VUC Instruction.cpp destructor __tcf_1 original 46 bytes, current 45/46.

Reorder genuine independent InstructionList method definitions within a
plausible 2004-era TU. GCC3.3.3 source emission order changes register
allocation state across functions. No instruction padding, dummy functions,
asm or hard register assignments. Selected original refs are checked.
"""
from pathlib import Path
import itertools,subprocess,importlib.util,json,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location("vuc_match",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
c=json.loads((R/"vuc/config/instruction_arena_cleanup_unit_oct14_probe.json").read_text())
src=(R/"vuc/probes/instruction_list_arena_unit_oct14.cpp").read_text()
src=src.replace('#include "../src/MLGameString.h"', '#include "'+str(R/'vuc/src/MLGameString.h')+'"')
defstarts=[
"void InstructionList::Add(Instruction *instruction)",
"void InstructionList::AddL(",
"MLGameString InstructionList::GenLabel(",
"MLGameString InstructionList::GenTmp(",
"void InstructionList::Print(",
"char *Instruction::instrData;",
"const char *Instruction::ToString()",
"void *Instruction::operator new(",
"struct InstructionArenaCleanup {",
]
indices=[src.index(x) for x in defstarts]
assert indices==sorted(indices)
chunks=[src[indices[i]:indices[i+1]] for i in range(8)]
prefix=src[:indices[0]];suffix=src[indices[8]:]
# Keep the Instruction static data and arena cleanup code in fixed locations;
# rearrange only class member function definitions and harmless real methods.
method=[0,1,2,3,4]
candidate_orders=[
 (0,1,2,3,4),(0,1,3,2,4),(0,4,1,2,3),(0,1,4,2,3),
 (1,0,2,3,4),(2,3,0,1,4),(3,2,0,1,4),
 (4,0,1,2,3),(4,1,0,2,3),(4,2,3,1,0),
]
# Sample a reproducible diverse sweep of up to 60 distinct 5-method
# permutations, no randomization, no fake neighboring definitions.
for perm in itertools.permutations(method):
 if len(candidate_orders)>=60:break
 if perm not in candidate_orders:candidate_orders.append(perm)
flags={
 'original':['-O2','-frename-registers','-fuse-cxa-atexit'],
 'plain':['-O2','-fuse-cxa-atexit'],
 'j1':['-O2','-frename-registers','-fuse-cxa-atexit','-falign-jumps=1'],
 'j4':['-O2','-frename-registers','-fuse-cxa-atexit','-falign-jumps=4'],
 'nosched2':['-O2','-frename-registers','-fuse-cxa-atexit','-fno-schedule-insns2'],
 'nocross':['-O2','-frename-registers','-fuse-cxa-atexit','-fno-crossjumping'],
}
T=Path('/tmp/vuc-binutils215/vuc-tcf-real-neighbor-order-oct15e');T.mkdir(parents=True,exist_ok=True)
rows=[];lengths=collections.Counter();best=None
for perm,(flag,opts) in itertools.product(candidate_orders,flags.items()):
 name=''.join(map(str,perm))+'-'+flag
 source=prefix+''.join(chunks[i] for i in perm)+''.join(chunks[i] for i in (5,6,7))+suffix
 cpp=T/(name+'.cpp');obj=T/(name+'.o');cpp.write_text(source)
 p=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if p.returncode:rows.append(dict(tag=name,error=p.stderr[-100:]));continue
 try:d=m.run(obj,R/"vuc/config/instruction_arena_cleanup_unit_oct14_probe.json")
 except ValueError as e:rows.append(dict(tag=name,error=str(e)[:160]));continue
 row=dict(tag=name,length=d['length'],match=d['exact_bytes'],relocations=d['relocations'],prefix=next(iter(d['different_offsets']),d['length']))
 rows.append(row);lengths[row['length']]+=1
 if row['length']==46 and row['relocations']==3 and (best is None or (row['match'],row['prefix'])>(best['match'],best['prefix'])):
  best=row;(T/'best.cpp').write_text(source);(T/'best.o').write_bytes(obj.read_bytes());(T/'best.flags').write_text(' '.join(opts)+'\n')
  if row['match']==46:
   print('EXACT',json.dumps(row),flush=True)
   (T/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
   raise SystemExit(0)
(T/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
print(json.dumps(dict(attempted=len(rows),sizes=dict(lengths),best=best,top=sorted((r for r in rows if 'match' in r),key=lambda r:r['match'],reverse=True)[:10]),indent=2))

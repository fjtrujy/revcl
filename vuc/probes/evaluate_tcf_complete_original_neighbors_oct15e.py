#!/usr/bin/env python3
"""VUC original 2004 Instruction.cpp real predecessor reconstruction.

Test __tcf_1's one dead scratch pop (original pop ecx vs GCC pop ebx)
with *actual accepted C++03 InstructionList method definitions* in
original ELF source-order, including variadic Add, AddB and 162-byte exact
AddLineComment. Test genuine method order/visibility, no dummy functions.
"""
from pathlib import Path
import itertools,json,importlib.util,subprocess,collections
R=Path(__file__).resolve().parents[2];s=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
canon=(R/'vuc/src/InstructionListLineCommentUnit.cpp').read_text()
arena=(R/'vuc/probes/instruction_list_arena_unit_oct14.cpp').read_text()
head=canon[:canon.index('void InstructionList::Add(Instruction *instruction)')]
head=head.replace('#include <ostream>','#include <iostream>\n#include <new>')
head=head.replace('#include "MLGameString.h"','#include "'+str(R/'vuc/src/MLGameString.h')+'"')
head=head.replace('    static void *operator new(std::size_t);','    static char *instrData;\n    static void *operator new(std::size_t);')
head=head.replace('    void AddLineComment(int line);', '    void AddLineComment(int line);\n    void GenerateImage(unsigned char *);')
def section(src,marker):
 a=src.index(marker);b=src.index('{',a);level=0
 for i in range(b,len(src)):
  if src[i]=='{':level+=1
  elif src[i]=='}':
   level-=1
   if level==0:return src[a:i+1]+'\n'
 raise ValueError(marker)
sources=[
('add',section(canon,'void InstructionList::Add(Instruction *instruction)')),
('addv',section(canon,'void InstructionList::Add(const char *format')),
('addb',section(canon,'void InstructionList::AddB(const char *format')),
('addl',section(canon,'void InstructionList::AddL(')),
('line',section(canon,'void InstructionList::AddLineComment(int line)')),
('genimage','void InstructionList::GenerateImage(unsigned char *) {}\n'),
('genlabel',section(canon,'MLGameString InstructionList::GenLabel(')),
('gentmp',section(canon,'MLGameString InstructionList::GenTmp(')),
('print',section(canon,'void InstructionList::Print(')),
('tostring',section(arena,'const char *Instruction::ToString()')),
('operatornew',section(arena,'void *Instruction::operator new(')),
]
methods=dict(sources)
cleanup=arena[arena.index('struct InstructionArenaCleanup {'):]
charptr='char *Instruction::instrData;\n'
original=[x[0] for x in sources]
# Variants use only original known existing genuine member method definitions
# and retain the static arena initializer/destructor semantic functions.
orders=[original]
for swap in ((0,1),(1,2),(2,3),(3,4),(4,5),(5,6),(6,7),(7,8),(8,9),(9,10),(2,4),(5,8),(0,8)):
 p=original[:];p[swap[0]],p[swap[1]]=p[swap[1]],p[swap[0]];orders.append(p)
orders+= [
 ['add','addl','genlabel','gentmp','print','addv','addb','line','genimage','tostring','operatornew'],
 ['add','addv','addb','line','addl','genimage','genlabel','gentmp','print','tostring','operatornew'],
 ['add','addv','addb','addl','line','genimage','gentmp','genlabel','print','operatornew','tostring'],
]
flags={
 'original':['-O2','-frename-registers','-fuse-cxa-atexit'],
 'j1':['-O2','-frename-registers','-fuse-cxa-atexit','-falign-jumps=1'],
 'j4':['-O2','-frename-registers','-fuse-cxa-atexit','-falign-jumps=4'],
 'plain':['-O2','-fuse-cxa-atexit'],
 'no-regmove':['-O2','-frename-registers','-fuse-cxa-atexit','-fno-regmove'],
 'no-sched2':['-O2','-frename-registers','-fuse-cxa-atexit','-fno-schedule-insns2'],
}
T=Path('/tmp/vuc-binutils215/vuc-tcf-authentic-all-neighbors-oct15e');T.mkdir(exist_ok=True,parents=True)
rows=[];sizes=collections.Counter();best=None
for j,ordering in enumerate(orders):
 for placement in ('after-methods','before-methods'):
  # declaration and remaining implementation remain original C++03
  ordered='\n'.join(methods[name] for name in ordering)
  source=head+charptr+(ordered+'\n'+cleanup if placement=='after-methods' else cleanup+'\n'+ordered)
  for flag,opt in flags.items():
   tag=f'{j:02d}-{placement}-{flag}'
   cpp=T/(tag+'.cpp');obj=T/(tag+'.o');cpp.write_text(source)
   p=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opt,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
   if p.returncode:rows.append(dict(tag=tag,error=p.stderr[-145:]));continue
   try:res=m.run(obj,R/'vuc/config/instruction_arena_cleanup_unit_oct14_probe.json')
   except ValueError as e:rows.append(dict(tag=tag,error=str(e)[:155]));continue
   row=dict(tag=tag,length=res['length'],match=res['exact_bytes'],relocations=res['relocations'],prefix=next(iter(res['different_offsets']),res['length']))
   rows.append(row);sizes[row['length']]+=1
   if row['length']==46 and row['relocations']==3 and (best is None or (row['match'],row['prefix'])>(best['match'],best['prefix'])):
    best=row;(T/'best.cpp').write_text(source);(T/'best.flags').write_text(' '.join(opt)+'\n');(T/'best.o').write_bytes(obj.read_bytes())
    if row['match']==46:
     print('EXACT',json.dumps(row),flush=True)
     (T/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
     raise SystemExit(0)
(T/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
print(json.dumps(dict(attempted=len(rows),sizes=dict(sizes),best=best,top=sorted((r for r in rows if 'match' in r),key=lambda r:r['match'],reverse=True)[:12]),indent=2))

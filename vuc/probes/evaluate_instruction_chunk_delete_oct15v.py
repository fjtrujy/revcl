#!/usr/bin/env python3
"""Authored arena-node ownership: scalar delete vs raw operator delete.

The original 0x2004-byte trailing chain link is modeled as a real member.
Neither instruction encodings nor ELF addresses are patched.
"""
from pathlib import Path
import subprocess,importlib.util
R=Path(__file__).resolve().parents[1];O=Path('/tmp/vuc-instruction-chunk-delete-oct15v');O.mkdir(exist_ok=True)
headers={
 'char': '#include <iostream>\n#include <new>\nstruct Instruction { static char *instrData; };\nchar *Instruction::instrData;\n',
 'unsigned-char': '#include <iostream>\n#include <new>\nstruct Instruction { static unsigned char *instrData; };\nunsigned char *Instruction::instrData;\n',
 'struct': '#include <iostream>\n#include <new>\nstruct InstructionChunk { char buffer[0x2004]; InstructionChunk *previous; };\nstruct Instruction { static InstructionChunk *instrData; };\nInstructionChunk *Instruction::instrData;\n',
 'struct-data': '#include <iostream>\n#include <new>\nstruct InstructionChunk { unsigned char buffer[0x2004]; InstructionChunk *previous; };\nstruct Instruction { static InstructionChunk *instrData; };\nInstructionChunk *Instruction::instrData;\n',
}
for typ,head in headers.items():
 ptype='InstructionChunk' if typ.startswith('struct') else ('unsigned char' if typ=='unsigned-char' else 'char')
 nextexpr='current->previous' if typ.startswith('struct') else '*(('+ptype+'**)(current+0x2004))'
 for lifetime in ('delete','global-delete','reference','for','do'):
  deletion='delete current' if lifetime=='delete' else '::operator delete(current)'
  if lifetime=='reference':body='''%s *&link=Instruction::instrData; while(link){ %s *current=link; %s *next=%s; %s; link=next; }'''%(ptype,ptype,ptype,nextexpr,deletion)
  elif lifetime=='for':body='''for(%s *current=Instruction::instrData;current;current=Instruction::instrData){ %s *next=%s; %s; Instruction::instrData=next; }'''%(ptype,ptype,nextexpr,deletion)
  elif lifetime=='do':body='''%s *current=Instruction::instrData; if(current) do { %s *next=%s; %s; Instruction::instrData=current=next; } while(current);'''%(ptype,ptype,nextexpr,deletion)
  else:body='''while(Instruction::instrData){ %s *current=Instruction::instrData; %s *next=%s; %s; Instruction::instrData=next; }'''%(ptype,ptype,nextexpr,deletion)
  name=typ+'-'+lifetime;code=head+'struct InstructionArenaCleanup { ~InstructionArenaCleanup() { '+body+' } };\nstatic InstructionArenaCleanup cleanup;\n';path=O/(name+'.cpp');path.write_text(code)
sp=importlib.util.spec_from_file_location('match',R/'scripts/match_probe.py');m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
for path in sorted(O.glob('*.cpp')):
 for cc in ('jan25',):
  for suffix,fl in [('rename',['-O2','-frename-registers','-fuse-cxa-atexit'])]:
   obj=path.with_name(path.stem+'-'+cc+'-'+suffix+'.o');p=subprocess.run(['/tmp/vuc-binutils215/g++-'+cc+'-as215',*fl,'-c',str(path),'-o',str(obj)],capture_output=True,text=True)
   if p.returncode:print('COMPILE_FAIL',path.stem,p.stderr[-125:],flush=True);continue
   try:
    score=m.run(obj,R/'config/instruction_arena_cleanup_probe.json');print('RESULT',path.stem,score['exact_bytes'],score['length'],score.get('different_offsets',[])[:13],flush=True)
    if score['exact']:print('BYTE_EXACT',path,flush=True);(O/'EXACT.txt').write_text(str(path)+'\n')
   except Exception as e:print('REJECT',path.stem,str(e)[:155],flush=True)

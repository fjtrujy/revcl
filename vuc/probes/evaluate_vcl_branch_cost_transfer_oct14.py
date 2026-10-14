#!/usr/bin/env python3
"""Try the historically evidenced VCL branch-cost knob under VUC GCC 3.3.3.

VUC is a different compiler; treat matching knobs as hypotheses only.
Examine authenticated TranslateConst, VFormatString and actual struct lengths.
"""
from pathlib import Path
import subprocess,importlib.util,itertools,json
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/vcl-branchcost-controls-oct14')
T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location('match',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
functions=[
('const',R/'vuc/probes/translate_const_glibc_oct14.cpp',R/'vuc/config/translate_const_oct14_probe.json','_Z14TranslateConstRK12MLGameStringRK4Type',290),
('vformat',R/'vuc/probes/ml_temp_vformat.cpp',R/'vuc/config/ml_temp_vformat_probe.json','_ZN16MLTempStringData13VFormatStringEPKcPc',238),
('linecomment',R/'vuc/probes/instruction_line_with_unit_oct13.cpp',R/'vuc/config/instruction_add_line_comment_context_oct13_probe.json','_ZN15InstructionList14AddLineCommentEi',162),
]
scored=[]
for name,src,config,symname,target in functions:
 for snapshot,cxx in (('jan25','/tmp/vuc-binutils215/g++-jan25-as215'),('dec06','/tmp/vuc-binutils215/g++-dec06-as215')):
  for jumps in (1,2,4,8):
   for branchcost in range(5):
    flags=['-O2','-frename-registers',f'-falign-jumps={jumps}',f'-mbranch-cost={branchcost}']
    tag=f'{name}-{snapshot}-j{jumps}-b{branchcost}'
    obj=T/(tag+'.o')
    cp=subprocess.run([cxx,*flags,'-c',str(src),'-o',str(obj)],capture_output=True,text=True)
    if cp.returncode:
     print('COMPILER_ERROR',tag,cp.stderr[-110:],flush=True);continue
    nm=subprocess.check_output(['nm','-S',str(obj)],text=True)
    line=next((x for x in nm.splitlines() if x.endswith(symname)),'')
    size=int(line.split()[1],16) if line else -1
    if size!=target:
     scored.append((0,size,tag,'size'));continue
    # Functions carrying multiple literal ranges require previous
    # relocation authentication before code scoring. The VFormat and
    # line-comment manifests already include them.
    if name=='const':
     scored.append((0,size,tag,'original length; needs literal relocation mapping'))
     print('ORIGINAL_LENGTH',tag,obj,flush=True)
     continue
    try:
     score=m.run(obj,config)
     scored.append((score['exact_bytes'],size,tag,score['different_offsets'][:12]))
     if score['exact']:
      print('NEW_EXACT',tag,obj,flush=True);raise SystemExit(0)
    except ValueError as e:scored.append((0,size,tag,'REJECT '+str(e)[:105]))
  print('DONE',name,snapshot,jumps,flush=True)
print('TOP')
for x in sorted(scored,reverse=True)[:35]:print(x,flush=True)

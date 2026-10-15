#!/usr/bin/env python3
"""Authentic C++03 Translate(Type) switch/fallthrough controls from ELF.

The original 352-byte body begins by testing type kind against 2 twice,
branches signed >2 to kind 3, decrements kind for case 1, and shares
the UInt constructor between direct type 2 and nonzero-kind-1 flags.
Only vary actual source case ownership and ordinary lifetime statements.
"""
from pathlib import Path
import subprocess,json,importlib.util
R=Path(__file__).resolve().parents[2]/'vuc'
O=Path('/tmp/vuc-binutils215/translate-switch-ownership-oct15q')
O.mkdir(exist_ok=True)
src=(R/'probes/TranslateTypeFullOct15q.cpp').read_text()
start=src.index('    switch (kind) {')
end=src.index('    if ((type.flags & 0x100)',start)
cases={
 'case1':'''        case 1:
          if (!((const unsigned char *)&type)[15])
              return new IR::Int_Data;
          return new IR::UInt_Data;
''',
 'case2':'''        case 2:
          return new IR::UInt_Data;
''',
 'case3':'''        case 3:
          return new IR::Float_Data(((int)type.width)>>10);
''',
}
variants={
 'selected':src,
 '2-1-3':src[:start]+'    switch (kind) {\n'+cases['case2']+cases['case1']+cases['case3']+'    }\n'+src[end:],
 '3-1-2':src[:start]+'    switch (kind) {\n'+cases['case3']+cases['case1']+cases['case2']+'    }\n'+src[end:],
 '2-3-1':src[:start]+'    switch (kind) {\n'+cases['case2']+cases['case3']+cases['case1']+'    }\n'+src[end:],
 '1-3-2':src[:start]+'    switch (kind) {\n'+cases['case1']+cases['case3']+cases['case2']+'    }\n'+src[end:],
 '3-2-1':src[:start]+'    switch (kind) {\n'+cases['case3']+cases['case2']+cases['case1']+'    }\n'+src[end:],
 'shared-uint-goto':src[:start]+'''    switch(kind) {
        case 1:
          if (((const unsigned char *)&type)[15]) goto make_uint;
          return new IR::Int_Data;
        case 2:
        make_uint:
          return new IR::UInt_Data;
        case 3:
          return new IR::Float_Data(((int)type.width)>>10);
    }
'''+src[end:],
 'explicit-kind-branch':src[:start]+'''    if (kind == 2 || (kind == 1 && ((const unsigned char *)&type)[15]))
        return new IR::UInt_Data;
    if (kind == 1)
        return new IR::Int_Data;
    if (kind == 3)
        return new IR::Float_Data(((int)type.width)>>10);
'''+src[end:],
}
spec=importlib.util.spec_from_file_location('m',R/'scripts/match_probe.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
cfg=R/'config/translate_type_partial_oct15q.json'
rows=[]
for tag,source in variants.items():
 cpp=O/(tag+'.cpp');cpp.write_text(source)
 for snapshot in ('jan25','dec06'):
  obj=O/(tag+'-'+snapshot+'.o')
  p=subprocess.run(['/tmp/vuc-binutils215/g++-'+snapshot+'-as215','-O2','-frename-registers','-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
  row={'source':tag,'snapshot':snapshot}
  if p.returncode:row['error']=p.stderr[-150:]
  else:
   nm=subprocess.run(['nm','-S','--defined-only',str(obj)],capture_output=True,text=True)
   sym=[x.split() for x in nm.stdout.splitlines() if len(x.split())>=4 and x.split()[3]=='_Z9TranslateRK4Typeji']
   row['length']=int(sym[0][1],16) if sym else -1
   if row['length']==352:
    try:
     v=m.run(obj,cfg)
     row['matching']=v['exact_bytes']
     row['relocations']=v['relocations']
    except ValueError as e:row['error']=str(e)
  rows.append(row)
  print(row,flush=True)
(O/'results.json').write_text(json.dumps(rows,indent=2)+'\n')

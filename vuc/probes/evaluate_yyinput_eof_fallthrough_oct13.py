#!/usr/bin/env python3
"""Test historical Flex yyinput EOF case fallthrough and common-return C++.

The original case-2 branch calls virtual yyrestart then falls through to
the case-1 EOF return site. Preserve genuine Flex state manipulation.
"""
from pathlib import Path
import subprocess,importlib.util
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/yyinput-eof-order-oct13');T.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
source=(R/'vuc/src/YyInput.cpp').read_text()
cases=source[source.index('            case 2:'):source.index('            case 0:')]
choices={
 'separate-returns':'''            case 1:
                return -1;
            case 2:
                yyrestart(yyin);
                return -1;
''',
 'baseline-exact':cases,
 'fallthrough':'''            case 2:
                yyrestart(yyin);
            case 1:
                return -1;
''',
 'fallthrough-explicit':'''            case 2:
                yyrestart(yyin);
                // no break, restart still reaches EOF
            case 1:
                return -1;
''',
 'goto-eof':'''            case 2:
                yyrestart(yyin);
                goto eof;
            case 1:
                goto eof;
''',
 'goto-after':'''            case 1:
                goto eof;
            case 2:
                yyrestart(yyin);
                goto eof;
''',
 'eof-set':'''            case 2:
                yyrestart(yyin);
            case 1:
                c = -1;
                goto done;
''',
}
outcomes=[]
for name,case in choices.items():
 for z in ('normal','return-direct','var-c'):
  body=source.replace(cases,case)
  if 'goto eof' in case:
   body=body.replace('    return c;\n}', '    return c;\neof:\n    return -1;\n}')
  if 'goto done' in case:
   body=body.replace('    return c;\n}', 'done:\n    return c;\n}')
  if z=='return-direct':
   body=body.replace('    c = (unsigned char)*yy_c_buf_p;', '    c = (unsigned char)*yy_c_buf_p;')
   body=body.replace('    return c;', '    return (unsigned char)c;')
  if z=='var-c':
   body=body.replace('    c = (unsigned char)*yy_c_buf_p;', '    c = *yy_c_buf_p;')
  p=T/(name+'-'+z+'.cpp');p.write_text(body)
  for opt in ('j1','j4','plain','default'):
   flags=['-O2']+(['-frename-registers'] if opt!='plain' else [])
   if opt in ('j1','j4'):flags+=['-falign-jumps='+('1' if opt=='j1' else '4')]
   o=T/(p.stem+'-'+opt+'.o')
   cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*flags,'-c',str(p),'-o',str(o)],capture_output=True,text=True)
   if cp.returncode:
    print('ERROR',name,z,opt,cp.stderr[-90:],flush=True);continue
   n=subprocess.check_output(['nm','-S',str(o)],text=True)
   s=next((x for x in n.splitlines() if x.endswith('_ZN11yyFlexLexer7yyinputEv')),'')
   length=int(s.split()[1],16) if s else -1
   if length==170:
    try:
     res=m.run(o,R/'vuc/config/yyinput_reconstruct_oct13_probe.json')
     outcomes.append((res['exact_bytes'],length,name,z,opt,res['different_offsets'][:12]))
     if res['exact']:print('EXACT',p,opt,flush=True);raise SystemExit(0)
    except ValueError as e:outcomes.append((0,length,name,z,opt,str(e)[:70]))
   else:outcomes.append((0,length,name,z,opt,'length'))
  print('DONE',name,z,flush=True)
print('BEST')
for item in sorted(outcomes,reverse=True)[:28]:print(item)

#!/usr/bin/env python3
"""VUC script_error 155/161: authentic iostream C++03 register-lifetime sources.
Six differing original bytes all encode scratch-register selection before
three char* stream insertions. Compile genuine compound expressions and
local stream/literal aliases with GCC 3.3.3. No register binding or asm.
"""
from pathlib import Path
import importlib.util,itertools,subprocess,json,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=R/'vuc/config/script_error_probe.json'
base=(R/'vuc/probes/script_error.cpp').read_text()
start=base.index('    std::cout << currFileName;')
end=base.index('\n}',start)
pre=base[:start];post=base[end:]
forms={
 'base':'std::cout << currFileName;\n    std::cout << "(" << script_line << "):" << message << std::endl;',
 'single-chain':'std::cout << currFileName << "(" << script_line << "):" << message << std::endl;',
 'separate-open':'std::cout << currFileName;\n    std::cout << "(";\n    std::cout << script_line << "):" << message << std::endl;',
 'split-trailing':'std::cout << currFileName;\n    std::cout << "(" << script_line;\n    std::cout << "):" << message << std::endl;',
 'split-message':'std::cout << currFileName;\n    std::cout << "(" << script_line << "):";\n    std::cout << message << std::endl;',
 'localstream':'std::ostream &stream = std::cout;\n    stream << currFileName;\n    stream << "(" << script_line << "):" << message << std::endl;',
 'pointerstream':'std::ostream *stream = &std::cout;\n    *stream << currFileName;\n    *stream << "(" << script_line << "):" << message << std::endl;',
 'endl-as-function':'std::cout << currFileName;\n    std::cout << "(" << script_line << "):" << message;\n    std::endl(std::cout);',
 'saved-begin-stream':'std::cout << currFileName;\n    std::ostream& stream = std::cout << "(";\n    stream << script_line << "):" << message << std::endl;',
 'saved-message':'const char *const str = message;\n    std::cout << currFileName;\n    std::cout << "(" << script_line << "):" << str << std::endl;',
 'saved-curr':'const char *const file = currFileName;\n    std::cout << file;\n    std::cout << "(" << script_line << "):" << message << std::endl;',
 'saved-line':'const int number = script_line;\n    std::cout << currFileName;\n    std::cout << "(" << number << "):" << message << std::endl;',
 'two-stream':'std::cout << currFileName;\n    (std::cout << "(" << script_line << "):") << message << std::endl;',
 'stream-explicit':'std::operator<<(std::cout,currFileName);\n    std::cout << "(" << script_line << "):" << message << std::endl;',
}
# Independent lifetime-changing aliases for the two real literal inserts.
prefixes={
 'none':'',
 'open':'const char* const opener="(";\n    ',
 'closing':'const char* const closer="):";\n    ',
 'both':'const char* const opener="(";\n    const char* const closer="):";\n    ',
 'stream':'std::ostream &destination=std::cout;\n    ',
 'open-before':'const char *begin = "(";\n    ',
}
flags={
 'original':['-O2','-frename-registers','-falign-jumps=1'],
 'plain':['-O2','-falign-jumps=1'],
 'j4':['-O2','-frename-registers','-falign-jumps=4'],
 'j8':['-O2','-frename-registers','-falign-jumps=8'],
 'no-gcse':['-O2','-frename-registers','-falign-jumps=1','-fno-gcse'],
 'no-reorder':['-O2','-frename-registers','-falign-jumps=1','-fno-reorder-blocks'],
 'no-regmove':['-O2','-frename-registers','-falign-jumps=1','-fno-regmove'],
}
T=Path('/tmp/vuc-binutils215/vuc-script-error-lifetimes-oct15d');T.mkdir(parents=True,exist_ok=True)
rows=[];sizes=collections.Counter();best=None
for (name,body),(prefix,prep),(fn,opts) in itertools.product(forms.items(),prefixes.items(),flags.items()):
 tag=f'{name}-{prefix}-{fn}'
 expanded=body.replace('"("','opener') if prefix in ('open','both','open-before') else body
 if prefix=='open-before':expanded=expanded.replace('opener','begin')
 if prefix in ('closing','both'):expanded=expanded.replace('"):"','closer')
 if prefix=='stream':expanded=expanded.replace('std::cout','destination')
 candidate=pre+'    '+prep+'    '+expanded+post
 cpp=T/(tag+'.cpp');obj=T/(tag+'.o');cpp.write_text(candidate)
 p=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if p.returncode:rows.append(dict(tag=tag,error=p.stderr[-95:]));continue
 try:res=m.run(obj,cfg)
 except ValueError as e:rows.append(dict(tag=tag,error=str(e)[:160]));continue
 row=dict(tag=tag,length=res['length'],match=res['exact_bytes'],relocations=res['relocations'],literal_bytes=res['verified_literal_bytes'],prefix=next(iter(res['different_offsets']),res['length']))
 rows.append(row);sizes[row['length']]+=1
 if row['length']==161 and row['relocations']==15 and row['literal_bytes']==5 and (best is None or (row['match'],row['prefix'])>(best['match'],best['prefix'])):
  best=row;(T/'best.cpp').write_text(candidate);(T/'best.flags').write_text(' '.join(opts)+'\n');(T/'best.o').write_bytes(obj.read_bytes())
  if row['match']==161:
   print('EXACT',json.dumps(row),flush=True)
   (T/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
   raise SystemExit(0)
(T/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
print(json.dumps(dict(attempted=len(rows),sizes=dict(sizes),best=best,top=sorted((r for r in rows if 'match' in r),key=lambda x:(x['match'],x['prefix']),reverse=True)[:12]),indent=2))

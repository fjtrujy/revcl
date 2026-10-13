#!/usr/bin/env python3
"""Real VUC MLTempString receiver/va_list lifetime hypotheses, 117-byte Set()."""
import subprocess,importlib.util
from pathlib import Path
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/mltemp-set-receivers-oct13');T.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
base=(R/'vuc/probes/ml_temp_set_return.cpp').read_text()
pre=base[:base.index('MLTempString *MLTempString::Set')].replace('"../src/MLTempStringData.h"','"'+str(R/'vuc/src/MLTempStringData.h')+'"')
start='''MLTempString *MLTempString::Set(const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
'''
end='''    va_end(arguments);
    return this;
}
'''
forms={
 'original':'text = MLTempStringData::Get()->VFormatString(format, (char *)arguments);',
 'selfptr':'MLTempString *self = this; self->text = MLTempStringData::Get()->VFormatString(format, (char *)arguments);',
 'selfref':'MLTempString &self = *this; self.text = MLTempStringData::Get()->VFormatString(format, (char *)arguments);',
 'formatptr':'const char *fmt = format; text = MLTempStringData::Get()->VFormatString(fmt, (char *)arguments);',
 'data-local':'MLTempStringData *data = MLTempStringData::Get(); text = data->VFormatString(format, (char *)arguments);',
 'data-format-local':'const char *fmt=format; MLTempStringData *data=MLTempStringData::Get(); text=data->VFormatString(fmt, (char *)arguments);',
 'result-local':'char *result = MLTempStringData::Get()->VFormatString(format, (char *)arguments); text = result;',
 'result-self':'MLTempString *self=this; char *result=MLTempStringData::Get()->VFormatString(format, (char *)arguments); self->text=result;',
 'result-ref':'MLTempString &self=*this; char *result=MLTempStringData::Get()->VFormatString(format, (char *)arguments); self.text=result;',
 'args-pointer':'char *args = (char *)arguments; text=MLTempStringData::Get()->VFormatString(format,args);',
 'all-locals':'MLTempString *self=this; const char *fmt=format; MLTempStringData *data=MLTempStringData::Get(); char *args=(char *)arguments; self->text=data->VFormatString(fmt,args);',
 'nested':'{ MLTempString *self=this; self->text=MLTempStringData::Get()->VFormatString(format,(char *)arguments); }',
 'cast-result':'text = (const char *)MLTempStringData::Get()->VFormatString(format,(char *)arguments);',
}
flgs={'plain':['-O2'],'rename':['-O2','-frename-registers'],'j4':['-O2','-frename-registers','-falign-jumps=4'],'noorder':['-O2','-frename-registers','-fno-reorder-blocks']}
rank=[]
for label,body in forms.items():
 p=T/(label+'.cpp');p.write_text(pre+start+'  '+body+'\n'+end)
 for fl,opts in flgs.items():
  obj=T/(label+'-'+fl+'.o')
  c=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(p),'-o',str(obj)],capture_output=True,text=True)
  if c.returncode:print(label,fl,'COMPILE',c.stderr[-80:],flush=True);continue
  s=subprocess.run(['nm','-S',str(obj)],capture_output=True,text=True).stdout
  sym=next((x for x in s.splitlines() if '_ZN12MLTempString3SetEPKcz' in x),'')
  length=int(sym.split()[1],16) if sym else 0
  if length==117:
   try:
    r=m.run(obj,R/'vuc/config/ml_temp_set_probe.json')
    rank.append((r['exact_bytes'],label,fl))
    if r['exact']:
     print('EXACT',label,fl,p,flush=True);raise SystemExit(0)
   except ValueError as e:print(label,fl,'MISMATCH',str(e)[:130],flush=True)
  else:rank.append((0,label,fl,length))
 print('checked',label,flush=True)
for item in sorted(rank,reverse=True)[:24]:print('BEST',item)

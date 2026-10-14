#!/usr/bin/env python3
"""2004 GCC source-lifetime investigation for VUC MLTempString default C2.

Original 101-byte C2 differs from the accepted 102-byte C1 by
singleton/memset operand lifetimes. Use actual MLTempStringData::Get
member, genuine formatted constructor and normal C++ expressions.
"""
import importlib.util,subprocess,itertools
from pathlib import Path
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/temp-default-c2-lifetime-oct14')
T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location('match',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
base=(R/'vuc/probes/ml_temp_default.cpp').read_text()
constructor=base[base.index('MLTempString::MLTempString()\n{'):]
variants={
'baseline':constructor,
'get-local':"""MLTempString::MLTempString()
{
    MLTempStringData *buffer = MLTempStringData::Get();
    text = buffer->FormatString("");
}
""",
'char-local':"""MLTempString::MLTempString()
{
    const char *empty = "";
    text = MLTempStringData::Get()->FormatString(empty);
}
""",
'const-char-ref':"""MLTempString::MLTempString()
{
    const char *const &empty = "";
    text = MLTempStringData::Get()->FormatString(empty);
}
""",
'format-local':"""MLTempString::MLTempString()
{
    char *result = MLTempStringData::Get()->FormatString("");
    text = result;
}
""",
'this-assign':"""MLTempString::MLTempString()
{
    this->text = MLTempStringData::Get()->FormatString("");
}
""",
'get-before-empty':"""MLTempString::MLTempString()
{
    MLTempStringData *buffer = MLTempStringData::Get();
    const char *const empty = "";
    this->text = buffer->FormatString(empty);
}
""",
'empty-before-get':"""MLTempString::MLTempString()
{
    const char *const empty = "";
    MLTempStringData *buffer = MLTempStringData::Get();
    this->text = buffer->FormatString(empty);
}
""",
'format-const-ref':"""MLTempString::MLTempString()
{
    const char *const &value = "";
    MLTempStringData *buffer = MLTempStringData::Get();
    text = buffer->FormatString(value);
}
""",
'owner-ref':"""MLTempString::MLTempString()
{
    MLTempStringData *const &buffer = MLTempStringData::Get();
    text = buffer->FormatString("");
}
""",
'owner-const':"""MLTempString::MLTempString()
{
    MLTempStringData *const buffer = MLTempStringData::Get();
    text = buffer->FormatString("");
}
""",
'constructor-temp':"""MLTempString::MLTempString()
{
    const char *name = "";
    MLTempStringData *buffer = MLTempStringData::Get();
    char *result = buffer->FormatString(name);
    this->text = result;
}
""",
'format-result-ref':"""MLTempString::MLTempString()
{
    char *const &result = MLTempStringData::Get()->FormatString("");
    text = result;
}
""",
'instance-named':"""MLTempString::MLTempString()
{
    MLTempStringData *data = MLTempStringData::Get();
    char *value = data->FormatString("");
    text = value;
}
""",
}
prefix=base[:base.index('MLTempString::MLTempString()\n{')]
prefix=prefix.replace('#include "../src/MLTempStringData.h"',
                      '#include "'+str(R/'vuc/src/MLTempStringData.h')+'"')
prefix=prefix.replace('#include "'+str(R/'vuc/src/MLTempStringData.h')+'"',
                      '#include "'+str(R/'vuc/src/MLTempStringData.h')+'"\n#include <cstdarg>')
prefix=prefix.replace('    MLTempString();\n};',
                      '    MLTempString();\n    MLTempString(const char *format, ...);\n};')
predecessor=(R/'vuc/probes/ml_temp_constructor_unit_oct12.cpp').read_text()
prestart=predecessor.index('MLTempString::MLTempString(const char *format, ...)')
preend=predecessor.index('MLTempString::MLTempString()\n{')
formatted=predecessor[prestart:preend]
flags={
'r':['-O2','-frename-registers'],
'plain':['-O2'],
'j1':['-O2','-frename-registers','-falign-jumps=1'],
'j4':['-O2','-frename-registers','-falign-jumps=4'],
'no-reorder':['-O2','-frename-registers','-fno-reorder-blocks'],
'no-sched':['-O2','-frename-registers','-fno-schedule-insns2'],
'no-regmove':['-O2','-frename-registers','-fno-regmove'],
'no-gcse':['-O2','-frename-registers','-fno-gcse'],
'no-cprop':['-O2','-frename-registers','-fno-cprop-registers'],
'j1-loop1':['-O2','-frename-registers','-falign-jumps=1','-falign-loops=1'],
'r-j2':['-O2','-frename-registers','-falign-jumps=2'],
}
scores=[]
for kind,source in variants.items():
 for context in ('isolated','formatted-predecessor'):
  code=prefix+(formatted if context=='formatted-predecessor' else '')+source
  path=T/(context+'-'+kind+'.cpp');path.write_text(code)
  for name,opts in flags.items():
   obj=T/(context+'-'+kind+'-'+name+'.o')
   cc=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(path),'-o',str(obj)],capture_output=True,text=True)
   if cc.returncode:
    print('FAIL',kind,context,name,cc.stderr[-120:],flush=True);continue
   sym=next((x for x in subprocess.check_output(['nm','-S',str(obj)],text=True).splitlines() if x.endswith('_ZN12MLTempStringC2Ev')),'')
   size=int(sym.split()[1],16) if sym else -1
   if size!=101:
    scores.append((0,size,context,kind,name,'size'));continue
   try:
    r=m.run(obj,R/'vuc/config/ml_temp_default_C2_probe.json')
    scores.append((r['exact_bytes'],size,context,kind,name,r['different_offsets'][:14]))
    if r['exact']:
     print('NEW_EXACT',path,name,flush=True);raise SystemExit(0)
   except ValueError as e:scores.append((0,size,context,kind,name,'reject '+str(e)[:90]))
  print('DONE',context,kind,flush=True)
print('BEST')
for s in sorted(scores,reverse=True)[:26]:print(s,flush=True)

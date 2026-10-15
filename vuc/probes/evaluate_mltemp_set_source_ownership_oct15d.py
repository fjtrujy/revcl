#!/usr/bin/env python3
"""VUC MLTempString::Set historical inline singleton initialization context.
The prior 111/117 partial's six wrong bytes select scratch regs for format
and memset. Try genuine C++03 constructor/Get lifetime forms only, preserving
real static guard/BSS references, memset and VFormatString call.
"""
from pathlib import Path
import itertools,subprocess,importlib.util,json,collections
R=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
cfg=R/'vuc/config/ml_temp_set_probe.json'
constructor={
 'original':"std::memset(bytes_, 'X', sizeof bytes_); bytes_[0x1fbf]=0; used_=0;",
 'literal-88':"std::memset(bytes_, 88, sizeof bytes_); bytes_[0x1fbf]=0; used_=0;",
 'explicit-size':"std::memset(bytes_, 'X', 0x2fc0); bytes_[0x1fbf]=0; used_=0;",
 'pointer-offset':"std::memset(&bytes_[0], 'X', sizeof bytes_); bytes_[0x1fbf]=0; used_=0;",
 'cast-void':"std::memset(static_cast<void*>(bytes_), 'X', sizeof bytes_); bytes_[0x1fbf]=0; used_=0;",
 'global-memset':"::memset(bytes_, 'X', sizeof bytes_); bytes_[0x1fbf]=0; used_=0;",
 'size-variable':"const std::size_t count=sizeof bytes_; std::memset(bytes_, 'X', count); bytes_[0x1fbf]=0; used_=0;",
 'char-value':"const unsigned char fill='X'; std::memset(bytes_, fill, sizeof bytes_); bytes_[0x1fbf]=0; used_=0;",
 'zero-pointer':"std::memset(bytes_, 'X', sizeof bytes_); *(bytes_+0x1fbf)=0; used_=0;",
 'pointer-member':"std::memset(bytes_, 'X', sizeof bytes_); bytes_[0x1fbf]=0; this->used_=0;",
 'zero-literal':"std::memset(bytes_, 'X', sizeof bytes_); bytes_[0x1fbf]='\\0'; used_=0;",
 'paren-value':"std::memset(bytes_, int('X'), sizeof(bytes_)); bytes_[0x1fbf] = char(0); used_ = unsigned(0);",
 'explicit-next':"char *pointer = bytes_; std::memset(pointer, 'X', sizeof bytes_); pointer[0x1fbf]=0; used_=0;",
}
getters={
 'original':"static MLTempStringData tsd; return &tsd;",
 'ptr-alias':"static MLTempStringData tsd; MLTempStringData *ptr=&tsd; return ptr;",
 'const-pointer':"static MLTempStringData tsd; MLTempStringData *const ptr=&tsd; return ptr;",
 'ref-alias':"static MLTempStringData tsd; MLTempStringData &ptr=tsd; return &ptr;",
 'this-cast':"static MLTempStringData tsd; return static_cast<MLTempStringData*>(&tsd);",
 'return-local':"static MLTempStringData tsd; MLTempStringData *p=&tsd; return p;",
}
set_forms={
 'original':"va_list arguments; va_start(arguments,format); text=MLTempStringData::Get()->VFormatString(format,(char*)arguments); va_end(arguments); return this;",
 'local-owner':"va_list arguments; va_start(arguments,format); MLTempStringData *data=MLTempStringData::Get(); text=data->VFormatString(format,(char*)arguments); va_end(arguments); return this;",
 'saved-result':"va_list arguments; va_start(arguments,format); char *result=MLTempStringData::Get()->VFormatString(format,(char*)arguments); text=result; va_end(arguments); return this;",
}
flags={
 'o2':['-O2'],
 'rename':['-O2','-frename-registers'],
 'j1':['-O2','-falign-jumps=1'],
 'j4':['-O2','-falign-jumps=4'],
 'no-gcse':['-O2','-fno-gcse'],
 'no-regmove':['-O2','-fno-regmove'],
}
header='''#include <cstring>
#include <cstdarg>
#include <cstddef>
class MLTempStringData {
public:
 static MLTempStringData *Get();
 MLTempStringData(){ $CTOR }
 char *VFormatString(const char *format, char *args);
private:
 char bytes_[0x2fc0];
 unsigned used_;
};
struct MLTempString {
 const char *text;
 MLTempString *Set(const char *format, ...);
};
inline MLTempStringData *MLTempStringData::Get() { $GET }
MLTempString *MLTempString::Set(const char *format, ...) { $SET }
'''
T=Path('/tmp/vuc-binutils215/mltemp-set-source-ownership-oct15d');T.mkdir(parents=True,exist_ok=True)
records=[];counts=collections.Counter();best=None
for (cn,ctor),(gn,get),(sn,method),(fn,opts) in itertools.product(constructor.items(),getters.items(),set_forms.items(),flags.items()):
 tag=f'{cn}-{gn}-{sn}-{fn}'
 code=header.replace('$CTOR',ctor).replace('$GET',get).replace('$SET',method)
 cpp=T/(tag+'.cpp');obj=T/(tag+'.o');cpp.write_text(code)
 p=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if p.returncode:records.append(dict(tag=tag,error=p.stderr[-120:]));continue
 try:d=m.run(obj,cfg)
 except ValueError as e:records.append(dict(tag=tag,error=str(e)[:145]));continue
 row=dict(tag=tag,length=d['length'],match=d['exact_bytes'],relocations=d['relocations'],prefix=next(iter(d['different_offsets']),d['length']))
 records.append(row);counts[row['length']]+=1
 if row['length']==117 and row['relocations']==8 and (best is None or (row['match'],row['prefix'])>(best['match'],best['prefix'])):
  best=row;(T/'best.cpp').write_text(code);(T/'best.o').write_bytes(obj.read_bytes());(T/'best.flags').write_text(' '.join(opts)+'\n')
  if row['match']==117:
   print('EXACT',json.dumps(row),flush=True)
   (T/'results.json').write_text(json.dumps(records,indent=2)+'\n')
   raise SystemExit(0)
(T/'results.json').write_text(json.dumps(records,indent=2)+'\n')
print(json.dumps(dict(attempted=len(records),sizes=dict(counts),best=best,top=sorted((r for r in records if 'match' in r),key=lambda r:(r['match'],r['prefix']),reverse=True)[:16]),indent=2))

#!/usr/bin/env python3
"""Original GCC 3.3.3 C++ STL scope iterator uninitialized-copy reconstruction.

Real std::__uninitialized_copy_aux specialization returning a vector iterator,
placement-new MLHashTable copy and catch cleanup; verify original-sized
ELF32 objects with all authentic exception and constructor relocations.
"""
from pathlib import Path
import importlib.util,itertools,subprocess,collections
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/scope-iter-lifetimes-oct14')
T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
header='''#include <vector>
#include <new>
#include <bits/stl_uninitialized.h>
#include "'''+str(R/'vuc/src/MLHashFunGameString.h')+'''"
#include "'''+str(R/'vuc/src/MLHashTable.h')+'''"
struct SymbolTableEntry;
typedef MLHashTable<MLGameString, SymbolTableEntry*, MLHashFun<MLGameString>, 513> Scope;
typedef std::vector<Scope> ScopeVector;
typedef ScopeVector::iterator Iter;
namespace std {
template<> Iter __uninitialized_copy_aux<Iter,Iter>(Iter first,Iter last,Iter result,__false_type)
{
    Iter current=result;
    try {
        for(;first!=last;++first,++current) {
            if(&*current) ::new((void*)&*current) Scope(*first);
        }
        return current;
    } catch(...) {
'''
foot='''        throw;
    }
}
}
'''
cleanup={
 'for':'        for(Iter ptr=result;ptr!=current;++ptr) ptr->clear();\n',
 'for-post':'        for(Iter ptr=result;ptr!=current;) { Iter old=ptr++; old->clear(); }\n',
 'while':'        Iter ptr=result;\n        while(ptr!=current) {ptr->clear(); ++ptr;}\n',
 'while-post':'        Iter ptr=result;\n        while(ptr!=current) {Iter old=ptr++; old->clear();}\n',
 'while-pre':'        Iter ptr=result;\n        while(ptr!=current) {Iter old=ptr; ++ptr; old->clear();}\n',
 'for-pointer':'        Scope *ptr=&*result,*end=&*current;\n        while(ptr!=end){ptr->clear();++ptr;}\n',
 'for-pointer-post':'        Scope *ptr=&*result,*end=&*current;\n        while(ptr!=end){Scope *old=ptr++;old->clear();}\n',
 'manual-index':'        for(unsigned i=0;i<(unsigned)(current-result);++i) result[i].clear();\n',
 'do-while':'        Iter ptr=result;\n        if(ptr!=current) do {ptr->clear();++ptr;}while(ptr!=current);\n',
 'while-ptr':'        Iter ptr=result;\n        while(ptr!=current){Scope *element=&*ptr; ++ptr; element->clear();}\n',
}
loop={
 'for':'        for(;first!=last;++first,++current) {if(&*current) ::new((void*)&*current) Scope(*first);}\n',
 'while':'        while(first!=last) {if(&*current) ::new((void*)&*current) Scope(*first); ++first; ++current;}\n',
 'while-post':'        while(first!=last) {if(&*current) ::new((void*)&*current) Scope(*first++); ++current;}\n',
 'for-direct':'        for(;first!=last;++first,++current) {::new((void*)&*current) Scope(*first);}\n',
}
sym='_ZSt24__uninitialized_copy_auxIN9__gnu_cxx17__normal_iteratorIP11MLHashTableI12MLGameStringP16SymbolTableEntry9MLHashFunIS3_ELi513EESt6vectorIS8_SaIS8_EEEESD_ET0_T_SF_SE_12__false_type'
flags={
'r':['-O2','-frename-registers'],
'p':['-O2'],
'r-j1':['-O2','-frename-registers','-falign-jumps=1'],
'r-j4':['-O2','-frename-registers','-falign-jumps=4'],
'r-label1':['-O2','-frename-registers','-falign-labels=1'],
'r-label2':['-O2','-frename-registers','-falign-labels=2'],
'r-noblocks':['-O2','-frename-registers','-fno-reorder-blocks'],
}
scores=[];sizes=collections.Counter();n=0
for (lname,lbody),(cname,cbody) in itertools.product(loop.items(),cleanup.items()):
 source=header.replace('''        for(;first!=last;++first,++current) {
            if(&*current) ::new((void*)&*current) Scope(*first);
        }
''',lbody)+cbody+foot
 p=T/(lname+'-'+cname+'.cpp');p.write_text(source)
 for flag,opts in flags.items():
  obj=T/(lname+'-'+cname+'-'+flag+'.o')
  proc=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(p),'-o',str(obj)],capture_output=True,text=True)
  n+=1
  if proc.returncode:
   print('COMPILE_ERROR',lname,cname,flag,proc.stderr[-110:],flush=True);continue
  line=next((ln for ln in subprocess.check_output(['nm','-S',str(obj)],text=True).splitlines() if ln.split()[-1:]==[sym]),'')
  size=int(line.split()[1],16) if line else -1;sizes[size]+=1
  if size!=149:continue
  try:
   res=m.run(obj,R/'vuc/config/scope_iterator_copy_oct12_probe.json')
   row=(res['exact_bytes'],lname,cname,flag,res['different_offsets'][:12],str(p),str(obj))
   scores.append(row)
   if res['exact']:print('NEW_EXACT',row,flush=True);raise SystemExit(0)
  except ValueError as e:print('REJECT',lname,cname,flag,str(e)[:105],flush=True)
 print('DONE',lname,cname,'n',n,'best',max((r[0] for r in scores),default=0),flush=True)
print('TESTED',n,'SIZES',sizes.most_common(20),'ORIGINAL_LENGTH_SCORED',len(scores))
for r in sorted(scores,reverse=True)[:25]:print('TOP',r,flush=True)

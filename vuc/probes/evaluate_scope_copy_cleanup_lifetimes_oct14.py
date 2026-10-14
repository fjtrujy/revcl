#!/usr/bin/env python3
"""Investigate VUC's original 120-byte STL scope-copy exception loop.

Build only historical C++ cleanup/lifetime source forms; genuine constructor,
clear, C++ exception and unwind relocations remain mandatory. Generated ELF
objects and candidates remain VM scratch until independently validated.
"""
from pathlib import Path
import itertools, importlib.util, subprocess, collections
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/scope-copy-cleanup-oct14');T.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location('match',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
original=(R/'vuc/probes/scope_uninitialized_cleanup_order.cpp').read_text()
for header in ('MLHashFunGameString.h', 'MLHashTable.h'):
 original=original.replace('#include "../src/'+header+'"',
                           '#include "'+str(R/'vuc/src'/header)+'"')
start=original.index('        Scope *ptr = result;')
end=original.index('        throw;',start)
prefix=original[:start];suffix=original[end:]
forms={
'preincrement':'''        Scope *ptr = result;
        while(ptr != current) { Scope *old=ptr++; old->clear(); }
''',
'preinc-saved':'''        Scope *ptr = result;
        while(ptr != current) { Scope *old=ptr; ++ptr; old->clear(); }
''',
'increment-before':'''        Scope *ptr = result;
        while(ptr != current) { ptr->clear(); ++ptr; }
''',
'for':'''        for(Scope *ptr=result;ptr!=current;++ptr)ptr->clear();
''',
'for-early':'''        for(Scope *ptr=result;ptr!=current;) { Scope *old=ptr++; old->clear(); }
''',
'for-saved':'''        for(Scope *ptr=result;ptr!=current;) { Scope *old=ptr; ++ptr; old->clear(); }
''',
'for-end':'''        Scope *ptr=result;
        for(;ptr!=current;) { Scope *old=ptr; ++ptr; old->clear(); }
''',
'while-invert':'''        Scope *ptr=result;
        while(current != ptr) { Scope *old=ptr++; old->clear(); }
''',
'while-test-start':'''        Scope *ptr=result;
        for(;;) { if (ptr == current) break; Scope *old=ptr++; old->clear(); }
''',
'while-test-end':'''        Scope *ptr=result;
        if (ptr != current) do { Scope *old=ptr++; old->clear(); } while(ptr != current);
''',
'early-if':'''        Scope *ptr=result;
        if (ptr == current) throw;
        while(ptr!=current) {Scope *old=ptr++;old->clear();}
''',
'volatile-ptr':'''        Scope *volatile ptr=result;
        while (ptr != current) { Scope *old=ptr; ptr=old+1; old->clear(); }
''',
}
preds={
'plain':lambda f:f,
'likely':lambda f:f.replace('ptr != current','__builtin_expect(ptr != current, 1)').replace('ptr==current','__builtin_expect(ptr==current,0)'),
'unlikely':lambda f:f.replace('ptr != current','__builtin_expect(ptr != current, 0)').replace('ptr==current','__builtin_expect(ptr==current,1)'),
}
flagsets={
'base':['-O2','-frename-registers'],
'label2':['-O2','-frename-registers','-falign-labels=2'],
'label4':['-O2','-frename-registers','-falign-labels=4'],
'j1':['-O2','-frename-registers','-falign-jumps=1'],
'j4':['-O2','-frename-registers','-falign-jumps=4'],
'j8':['-O2','-frename-registers','-falign-jumps=8'],
'noreorder':['-O2','-frename-registers','-fno-reorder-blocks'],
'plain':['-O2'],
'loop4':['-O2','-frename-registers','-falign-loops=4'],
'loop8':['-O2','-frename-registers','-falign-loops=8'],
}
best=[];sizes=collections.Counter();n=0
for (name,form),(pred,transform) in itertools.product(forms.items(),preds.items()):
 cpp=T/(name+'-'+pred+'.cpp')
 cpp.write_text(prefix+transform(form)+suffix)
 for flag,opts in flagsets.items():
  obj=T/(name+'-'+pred+'-'+flag+'.o')
  cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
  n+=1
  if cp.returncode:
   print('COMPILE_ERROR',name,pred,flag,cp.stderr[-100:],flush=True);continue
  nm=subprocess.check_output(['nm','-S',str(obj)],text=True)
  sym=next((ln for ln in nm.splitlines() if ln.split()[-1:]==['_ZSt24__uninitialized_copy_auxIP11MLHashTableI12MLGameStringP16SymbolTableEntry9MLHashFunIS1_ELi513EES7_ET0_T_S9_S8_12__false_type']),'')
  size=int(sym.split()[1],16) if sym else -1
  sizes[size]+=1
  if size!=120:continue
  try:
   r=m.run(obj,R/'vuc/config/scope_uninitialized_copy_probe.json')
   val=(r['exact_bytes'],name,pred,flag,r['different_offsets'][:15],str(cpp),str(obj))
   best.append(val)
   if r['exact']:print('NEW_EXACT',val,flush=True);raise SystemExit
  except ValueError as e:print('MISMATCH',name,pred,flag,str(e)[:85],flush=True)
 print('FINISHED',name,'attempts',n,'best',max((x[0] for x in best),default=None),flush=True)
print('SIZES',sizes.most_common(20),'TESTED',n,'ORIGINAL_LENGTH_SCORED',len(best),flush=True)
for row in sorted(best,reverse=True)[:25]:print('TOP',row,flush=True)

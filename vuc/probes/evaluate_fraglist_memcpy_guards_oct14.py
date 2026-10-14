#!/usr/bin/env python3
"""ASTNodeList<IR::Fragment_Data*>: preserve original GCC rep movsl while
recovering the surrounding 7-byte source-level list-growth guards.

Tests C++03 expressions with the *same* observable array-copy semantics.
Only strict original-length/relocation-scored variants may be selected.
"""
from pathlib import Path
import subprocess, itertools, importlib.util, collections
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/fraglist-memcpy-guards-oct14');T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
originals={k:(R/('vuc/probes/translate_frag_list_'+k+'_partial_improved_oct14.cpp')).read_text()
 for k in ('var','decl')}
old='''            if (old_entries)
                for (int j=0; j<length; ++j) entries[j] = old_entries[j];'''
assert all(old in x for x in originals.values())
new={
'memcpy':'if (old_entries) __builtin_memcpy(entries,old_entries,length*sizeof(T));',
'memcpy-old':'if (old_entries) __builtin_memcpy(entries,old_entries,old_size*sizeof(T));',
'guard-count':'if (old_entries && old_size) __builtin_memcpy(entries,old_entries,old_size*sizeof(T));',
'guard-count2':'if (old_entries && length) __builtin_memcpy(entries,old_entries,length*sizeof(T));',
'guard-count-unsigned':'if (old_entries && (unsigned)old_size != 0U) __builtin_memcpy(entries,old_entries,old_size*sizeof(T));',
'guard-sizegt0':'if (old_entries && old_size > 0) __builtin_memcpy(entries,old_entries,old_size*sizeof(T));',
'guard-null-count':'if (old_entries) { if (old_size) __builtin_memcpy(entries,old_entries,old_size*sizeof(T)); }',
'guard-bytes':'if (old_entries) { unsigned bytes=old_size*sizeof(T); if(bytes) __builtin_memcpy(entries,old_entries,bytes); }',
'guard-len-before':'if (old_size) { if(old_entries) __builtin_memcpy(entries,old_entries,old_size*sizeof(T)); }',
'guard-stride':'if (old_entries && old_size >= 1) __builtin_memcpy(entries,old_entries,old_size*sizeof(T));',
'guard-end':'if (old_entries && old_size > 0) { T *begin=old_entries,*end=begin+old_size; __builtin_memcpy(entries,begin,(end-begin)*sizeof(T)); }',
'guard-count-before':'if (old_size > 0 && old_entries) __builtin_memcpy(entries,old_entries,old_size*sizeof(T));',
}
flags={
'j1':['-O2','-frename-registers','-falign-jumps=1','-fno-reorder-blocks'],
'j2':['-O2','-frename-registers','-falign-jumps=2','-fno-reorder-blocks'],
'j4':['-O2','-frename-registers','-falign-jumps=4','-fno-reorder-blocks'],
'j8':['-O2','-frename-registers','-falign-jumps=8','-fno-reorder-blocks'],
'j4-b':['-O2','-frename-registers','-falign-jumps=4'],
'j4-p':['-O2','-falign-jumps=4','-fno-reorder-blocks'],
}
scores=[];sizes=collections.Counter();n=0
for kind,orig in originals.items():
 for mode,repl in new.items():
  cpp=T/(kind+'-'+mode+'.cpp')
  cpp.write_text(orig.replace(old,'            '+repl))
  for flag,opts in flags.items():
   obj=T/(kind+'-'+mode+'-'+flag+'.o')
   cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
   n+=1
   if cp.returncode:
    print('COMPILATION',kind,mode,flag,cp.stderr[-100:],flush=True);continue
   sym='_Z17TranslateFragListIP10ASTVarDeclE11ASTNodeListIPN2IR13Fragment_DataEERS2_IT_E' if kind=='var' else '_Z17TranslateFragListIP7ASTDeclE11ASTNodeListIPN2IR13Fragment_DataEERS2_IT_E'
   rows=subprocess.check_output(['nm','-S',str(obj)],text=True).splitlines()
   line=next((x for x in rows if x.split()[-1:]==[sym]),'')
   size=int(line.split()[1],16) if line else -1;sizes[size]+=1
   if size!=240:continue
   try:
    r=m.run(obj,R/('vuc/config/translate_frag_list_'+kind+'_oct14_probe.json'))
    item=(r['exact_bytes'],kind,mode,flag,r['different_offsets'][:14],str(cpp),str(obj))
    scores.append(item)
    if r['exact']:print('NEW_EXACT',item,flush=True);raise SystemExit
   except ValueError as e:print('REJECT',kind,mode,flag,str(e)[:100],flush=True)
  print('DONE',kind,mode,'max',max((x[0] for x in scores),default=0),flush=True)
print('TESTED',n,'SIZES',sizes.most_common(18),'MATCHED_ORIGINAL_LENGTH',len(scores),flush=True)
for item in sorted(scores,reverse=True)[:30]:print('TOP',item,flush=True)

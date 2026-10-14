#!/usr/bin/env python3
"""Transfer the 112/256 ASTExpr inlining/lifetime pattern to VUC
TranslateFragList<ASTDecl/ASTVarDecl>. Uses genuine C++03 methods,
real call symbols and fully authenticated ELF32 relocations.
"""
from pathlib import Path
import subprocess, importlib.util, itertools, collections
ROOT=Path(__file__).resolve().parents[2]
TMP=Path('/tmp/vuc-binutils215/fraglist-ast-expr-transfer-corrected-oct14')
TMP.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location('m',ROOT/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
original=(ROOT/'vuc/probes/translate_expr_list_partial_improved_oct14.cpp').read_text()
head=original[:original.index('ASTNodeList<IR::Expr_Data *> Translate(')]
head=head.replace('struct ASTExpr {};','struct ASTDecl {};\nstruct ASTVarDecl : ASTDecl {};').replace('IR::Expr_Data','IR::Fragment_Data').replace('struct Expr_Data;','struct Fragment_Data;')
head=head.replace('IR::Fragment_Data *Translate(ASTExpr *, unsigned, int);','IR::Fragment_Data *TranslateFrag(ASTDecl *);')
old='''            if (old_entries)
                for (int j=0; j<length; ++j) entries[j] = old_entries[j];'''
assert old in head
forms={
 'scalar':old,
 'memcpy':'            if (old_entries) __builtin_memcpy(entries,old_entries,length*sizeof(T));',
 'memcpy-saved':'            if (old_entries) __builtin_memcpy(entries,old_entries,old_size*sizeof(T));',
 'memcpy-dest':'            if (old_entries) { T *dst=entries; __builtin_memcpy(dst,old_entries,length*sizeof(T)); }',
 'memcpy-locals':'            if (old_entries) { T *dst=entries; T *src=old_entries; int count=old_size*4; __builtin_memcpy(dst,src,count); }',
 'memmove':'            if (old_entries) __builtin_memmove(entries,old_entries,length*sizeof(T));',
 'scalar-saved':'            if (old_entries) for (int j=0;j<old_size;++j) entries[j]=old_entries[j];',
}
loopforms={
 'exp-index':'''    int i=0;
    while (i<input.length) {
        int index=i; ++i;
        result.Add(TranslateFrag(input.entries[index]));
    }''',
 'postinc':'''    int i=0;
    while (i<input.length)
        result.Add(TranslateFrag(input.entries[i++]));''',
 'for':'''    for (int i=0; i<input.length; ++i)
        result.Add(TranslateFrag(input.entries[i]));''',
 'for-local':'''    for (int i=0; i<input.length; ++i) {
        IR::Fragment_Data *item=TranslateFrag(input.entries[i]);
        result.Add(item);
    }''',
 'index-local':'''    int i=0;
    while (i<input.length) {
        int index=i; ++i;
        IR::Fragment_Data *item=TranslateFrag(input.entries[index]);
        result.Add(item);
    }''',
 'pointer':'''    ASTDecl **current=(ASTDecl **)input.entries;
    ASTDecl **end=current+input.length;
    while(current!=end) result.Add(TranslateFrag(*current++));''',
 'volatile-index':'''    volatile int i=0;
    while (i<input.length) {
        int index=i; ++i;
        result.Add(TranslateFrag(input.entries[index]));
    }''',
}
flags={
 'r-nrj1':['-O2','-frename-registers','-falign-jumps=1','-fno-reorder-blocks'],
 'r-nrj2':['-O2','-frename-registers','-falign-jumps=2','-fno-reorder-blocks'],
 'r-nrj4':['-O2','-frename-registers','-falign-jumps=4','-fno-reorder-blocks'],
 'r-nrj8':['-O2','-frename-registers','-falign-jumps=8','-fno-reorder-blocks'],
 'r-j1':['-O2','-frename-registers','-falign-jumps=1'],
 'r-j4':['-O2','-frename-registers','-falign-jumps=4'],
 'p-j1':['-O2','-falign-jumps=1'],
 'p-j4':['-O2','-falign-jumps=4'],
}
scores=[];sizes=collections.Counter();attempted=0
for (form,add),(loop,body),kind in itertools.product(forms.items(),loopforms.items(),('var','decl')):
 typ='ASTVarDecl' if kind=='var' else 'ASTDecl'
 cpp=TMP/('-'.join((kind,form,loop))+'.cpp')
 cpp.write_text(head.replace(old,add)+
    '\ntemplate<class T> ASTNodeList<IR::Fragment_Data *> TranslateFragList(ASTNodeList<T> &input)\n{\n'
    '    ASTNodeList<IR::Fragment_Data *> result;\n'+body+'\n    return result;\n}\n'
    'template ASTNodeList<IR::Fragment_Data *> TranslateFragList<'+typ+' *>(ASTNodeList<'+typ+' *> &);\n')
 for label,opts in flags.items():
  o=TMP/('-'.join((kind,form,loop,label))+'.o')
  p=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(cpp),'-o',str(o)],capture_output=True,text=True)
  attempted+=1
  if p.returncode:
   print('ERROR',cpp.name,label,p.stderr[-100:],flush=True);continue
  sym='_Z17TranslateFragListIP10ASTVarDeclE11ASTNodeListIPN2IR13Fragment_DataEERS2_IT_E' if kind=='var' else '_Z17TranslateFragListIP7ASTDeclE11ASTNodeListIPN2IR13Fragment_DataEERS2_IT_E'
  row=next((x for x in subprocess.check_output(['nm','-S',str(o)],text=True).splitlines() if x.split()[-1:]==[sym]),'')
  size=int(row.split()[1],16) if row else -1
  sizes[size]+=1
  if size!=240:continue
  try:
   res=m.run(o,ROOT/('vuc/config/translate_frag_list_'+kind+'_oct14_probe.json'))
   rec=(res['exact_bytes'],kind,form,loop,label,res['different_offsets'][:11],str(cpp),str(o))
   scores.append(rec)
   if res['exact']:
    print('NEW_EXACT',rec,flush=True);raise SystemExit(0)
  except ValueError as e:print('REJECT',cpp.name,label,str(e)[:125],flush=True)
 if attempted%56==0:
  print('PROGRESS',attempted,'best',max((x[0] for x in scores),default=0),flush=True)
print('TOTAL',attempted,'LENGTHS',sizes.most_common(18),'SCORED',len(scores),flush=True)
print('TOP')
for r in sorted(scores,reverse=True)[:42]:print(r,flush=True)

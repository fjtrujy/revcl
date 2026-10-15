#!/usr/bin/env python3
"""VUC 135-byte GetNextWord: genuine return/word-buffer lifetime exploration.

Original GCC uses EAX for global position and ECX for input while current
C++03 uses ECX and EAX reversed. Test source-level return ownership, output
pointer declaration placement and char comparison type to change live ranges
without constraining registers or changing the word-delimiter semantics.
Relocation-site authentication and original function length are mandatory.
"""
from pathlib import Path
import itertools,subprocess,importlib.util,json,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=R/'vuc/config/get_next_word_partial_oct15e_probe.json'
base=(R/'vuc/probes/GetNextWordByteBoundsPartialOct15e.cpp').read_text()
base=base[base.index('extern int currentPosition;'):]
assert 'int position=currentPosition;' in base
assert '*dest=0;\n return input;' in base
# Source base: uses input directly in the original 23/135 candidate.
outer={
 'original':base,
 'pointer-copy':base.replace('int position=currentPosition;', 'int position=currentPosition; const char *scan=input;').replace('*input','*scan').replace('++input','++scan').replace('return input;','return scan;'),
 'pointer-first':base.replace('int position=currentPosition;', 'const char *scan=input; int position=currentPosition;').replace('*input','*scan').replace('++input','++scan').replace('return input;','return scan;'),
 'char-output-early':base.replace('int position=currentPosition;', 'char *dest=word; int position=currentPosition;').replace(' char *dest=word;\n for (;;)', ' for (;;)'),
 'output-early':base.replace('int position=currentPosition;', 'int position=currentPosition; char *dest=word;').replace(' char *dest=word;\n for (;;)', ' for (;;)'),
 'unsigned-pos':base.replace('int position=currentPosition;','unsigned int position=currentPosition;'),
 'reg-pos':base.replace('int position=currentPosition;','register int position=currentPosition;'),
 'unsigned-early':'',
}
outer.pop('unsigned-early')
output={
 'simple':'*dest=0;\n return $RET;',
 'comma':'return (*dest=0), $RET;',
 'saved-before':'const char *result=$RET; *dest=0; return result;',
 'saved-after':'*dest=0; const char *result=$RET; return result;',
 'const-ref':'const char *const &result=$RET; *dest=0; return result;',
 'volatile-pointer':'const char *volatile result=$RET; *dest=0; return result;',
 'cast-void':'*dest=0; return static_cast<const char*>($RET);',
 'cast-uchar':'*dest=0; return reinterpret_cast<const char*>(reinterpret_cast<const unsigned char*>($RET));',
 'dereference-order':'*dest=char(0); return $RET;',
 'return-scope':'{ const char* result=$RET; *dest=0; return result; }',
}
comparisons={
 'byte':'unsigned char c=*input;',
 'signed':'signed char c=*input;',
 'char':'char c=*input;',
 'int':'int c=(unsigned char)*input;',
 'intchar':'int c=(char)*input;',
 'unsigned-int':'unsigned int c=(unsigned char)*input;',
}
updates={
 'original':'*dest++=c;\n ++input;\n ++position;\n currentPosition=position;',
 'first-position':'*dest++=c;\n ++position;\n ++input;\n currentPosition=position;',
 'store-prefix':'*dest++=c;\n ++input;\n currentPosition=++position;',
 'copy-last':'*dest=c;\n ++input;\n ++position;\n currentPosition=position; ++dest;',
}
flags={
 'j16':['-O2','-frename-registers','-falign-jumps=16'],
 'j8':['-O2','-frename-registers','-falign-jumps=8'],
 'j4':['-O2','-frename-registers','-falign-jumps=4'],
}
T=Path('/tmp/vuc-binutils215/getnext-return-buffer-lifetime-oct15f');T.mkdir(exist_ok=True,parents=True)
results=[];sizes=collections.Counter();best=None
for o,src in outer.items():
 for out,epilogue in output.items():
  for typ,decl in comparisons.items():
   for up,update in updates.items():
    # diverse 640 bounded candidate recipes
    if len(results)>=650:break
    if o in ('pointer-copy','pointer-first') and out in ('const-ref','volatile-pointer','return-scope'):continue
    if o in ('char-output-early','output-early') and typ in ('unsigned-int','intchar'):continue
    tag=f'{o}-{out}-{typ}-{up}'
    inp='scan' if o.startswith('pointer') else 'input'
    variant=src
    assert 'unsigned char c=*'+inp+';' in variant
    variant=variant.replace('unsigned char c=*'+inp+';',decl.replace('*input','*'+inp))
    original_update='*dest++=c;\n ++'+inp+';\n ++position;\n currentPosition=position;'
    assert original_update in variant
    variant=variant.replace(original_update,update.replace('++input','++'+inp))
    expected='*dest=0;\n return '+inp+';'
    assert expected in variant
    variant=variant.replace(expected,epilogue.replace('$RET',inp))
    cpp=T/(tag+'.cpp');obj=T/(tag+'.o');cpp.write_text(variant)
    opt=flags['j16']
    proc=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opt,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
    if proc.returncode:results.append(dict(tag=tag,error=proc.stderr[-130:]));continue
    try:d=m.run(obj,cfg)
    except ValueError as e:results.append(dict(tag=tag,error=str(e)[:125]));continue
    row=dict(tag=tag,length=d['length'],match=d['exact_bytes'],relocations=d['relocations'],prefix=next(iter(d['different_offsets']),d['length']))
    results.append(row);sizes[d['length']]+=1
    if row['length']==135 and row['relocations']==5 and (best is None or (row['match'],row['prefix'])>(best['match'],best['prefix'])):
     best=row;(T/'best.cpp').write_text(variant);(T/'best.o').write_bytes(obj.read_bytes());(T/'best.flags').write_text(' '.join(opt)+'\n')
     if row['match']>23:print('IMPROVEMENT',json.dumps(row),flush=True)
     if row['match']==135:print('EXACT',json.dumps(row),flush=True);break
(T/'results.json').write_text(json.dumps(results,indent=2)+'\n')
print(json.dumps(dict(attempted=len(results),sizes=dict(sizes),strict_original_length=sum(x.get('length')==135 and x.get('relocations')==5 for x in results),best=best,top=sorted((r for r in results if 'match' in r),key=lambda r:(r['match'],r['prefix']),reverse=True)[:12]),indent=2))

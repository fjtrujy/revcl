#!/usr/bin/env python3
"""VUC GetNextWord original 135-byte scanner: C++03 switch tree reconstruction.

The archived ELF uses byte-sized two-level comparisons at 44 and 9,
including the original 0,9,32,44,46,91 delimiters. Contrast legitimate
switch statement types/control ownership with the recently selected
135-byte 23/135 boolean chain. All code is real C++03, GCC 3.3.3, as 2.15.
No artificial branches, invented relocations or padding.
"""
from pathlib import Path
import itertools,subprocess,importlib.util,json,collections
R=Path(__file__).resolve().parents[2];sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=R/'vuc/config/get_next_word_partial_oct15e_probe.json'
orig=(R/'vuc/probes/GetNextWordByteBoundsPartialOct15e.cpp').read_text()
src=orig[orig.index('extern int currentPosition;'):]
start=src.index('unsigned char c=*input;',src.index(' char *dest=word;'));end=src.index(' *dest++=c;',start)
old=src[start:end]
assert 'if(c==0||c==9||c==32||c==44||c==46||c==91)goto endword;' in old
switches={
 'char':'char c=*input;',
 'signed':'signed char c=*input;',
 'uchar':'unsigned char c=*input;',
 'int':'int c=(unsigned char)*input;',
 'int-signed':'int c=(char)*input;',
 'charref':'const char c=*input;',
}
decisions={
 'switch':'''switch(c) {
 case 0: case 9: case 32: case 44: case 46: case 91: goto endword;
 default: break;
}''',
 'switch-charlits':'''switch(c) {
 case '\\0': case '\\t': case ' ': case ',': case '.': case '[': goto endword;
 default: break;
}''',
 'switch-no-default':'''switch(c) {
 case 0: case 9: case 32: case 44: case 46: case 91: goto endword;
}''',
 'switch-close-first':'''switch(c) {
 case 44: case 46: case 91: case 32: case 9: case 0: goto endword;
}''',
 'switch-endreturn':'''switch(c) {
 case 0: case 9: case 32: case 44: case 46: case 91:
  *dest=0; return input;
}''',
 'range-cases':'''if(c==44)goto endword;
if(c>44) {
 if(c==46 || c==91)goto endword;
} else {
 if(c==9 || c==32 || c==0)goto endword;
}''',
 'range-ordered':'''if(c==44)goto endword;
if(c>44) {
 if(c==46)goto endword;
 if(c==91)goto endword;
} else {
 if(c==9)goto endword;
 if(c>9){if(c==32)goto endword;} else if(c==0)goto endword;
}''',
 'switch-split':'''switch(c) {
case 44:goto endword;
case 46: case 91: goto endword;
case 0: case 9: case 32:goto endword;
}''',
}
leads={
 'original':'',
 'above':'if (c <= 10) { if (c < 9) break; } else if (c != 32) break;',
 'inverted':'if(c > 10) {if(c != 32)break;} else if(c < 9)break;',
}
# Already named original lead has a const c with split branch; preserve
# existing then include natural alternatives.
leading_orig='if (c <= 10) { if (c < 9) break; }\n else if (c != 32) break;'
assert leading_orig in src
methods={
 'original':'*dest++=c;\n ++input;\n ++position;\n currentPosition=position;',
 'positionfirst':'*dest++=c;\n ++position;\n ++input;\n currentPosition=position;',
 'globalfirst':'*dest++=c;\n ++input;\n currentPosition=++position;',
 'charwrite':'*dest=c;\n ++position;\n ++input;\n currentPosition=position;\n ++dest;',
}
original_update='*dest++=c;\n ++input;\n ++position;\n currentPosition=position;'
assert original_update in src
options={
 'j16':['-O2','-frename-registers','-falign-jumps=16'],
 'j8':['-O2','-frename-registers','-falign-jumps=8'],
 'j4':['-O2','-frename-registers','-falign-jumps=4'],
 'plain16':['-O2','-falign-jumps=16'],
}
T=Path('/tmp/vuc-binutils215/getnext-switch-byte-tree-oct15f');T.mkdir(exist_ok=True,parents=True)
records=[];lengths=collections.Counter();best=None
for (tn,typed),(dn,decision),(wn,write),ln,flag in itertools.product(switches.items(),decisions.items(),methods.items(),('original','inverted'),('j16','j8')):
 if len(records)>=500:break
 tag='-'.join((tn,dn,wn,ln,flag))
 form=src[:start]+typed+'\n'+decision+src[end:]
 if wn!='original':form=form.replace(original_update,write)
 if ln=='inverted':form=form.replace(leading_orig,leads['inverted'])
 cpp=T/(tag+'.cpp');obj=T/(tag+'.o');cpp.write_text(form)
 proc=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*options[flag],'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if proc.returncode:records.append(dict(tag=tag,error=proc.stderr[-145:]));continue
 try:d=m.run(obj,cfg)
 except ValueError as e:records.append(dict(tag=tag,error=str(e)[:125]));continue
 row=dict(tag=tag,length=d['length'],match=d['exact_bytes'],relocations=d['relocations'],prefix=next(iter(d['different_offsets']),d['length']))
 records.append(row);lengths[d['length']]+=1
 if row['length']==135 and row['relocations']==5 and (best is None or (row['match'],row['prefix'])>(best['match'],best['prefix'])):
  best=row;(T/'best.cpp').write_text(form);(T/'best.o').write_bytes(obj.read_bytes());(T/'best.flags').write_text(' '.join(options[flag])+'\n')
  if row['match']>23:print('IMPROVE',json.dumps(row),flush=True)
  if row['match']==135:print('EXACT',json.dumps(row),flush=True);break
(T/'results.json').write_text(json.dumps(records,indent=2)+'\n')
print(json.dumps(dict(attempted=len(records),strict_length=sum(x.get('length')==135 and x.get('relocations')==5 for x in records),lengths=dict(lengths),best=best,top=sorted((x for x in records if 'match' in x),key=lambda x:(x['match'],x['prefix']),reverse=True)[:15]),indent=2))

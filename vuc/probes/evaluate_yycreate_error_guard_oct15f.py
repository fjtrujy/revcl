#!/usr/bin/env python3
"""VUC yyFlexLexer::yy_create_buffer 131/139: authentic C++03 failure-branch topology.

Test genuine branch predictions, negative guards and normal source labels
around both malloc failure checks. The original 139-byte body has five
authenticated relocations and 44 literal bytes; preserve both gates.
No artificial code, hard registers, inline assembly, or byte patches.
"""
from pathlib import Path
import itertools,importlib.util,subprocess,json,collections
R=Path(__file__).resolve().parents[2];s=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
src=(R/'vuc/probes/yy_create_buffer.cpp').read_text()
first='''    if (!b)
        LexerError("out of dynamic memory in yy_create_buffer()");'''
second='''    if (!b->buffer)
        LexerError("out of dynamic memory in yy_create_buffer()");'''
assert first in src and second in src
forms={
 'original':'if (!EXPR) LexerError(MSG);',
 'null-equal':'if (EXPR == 0) LexerError(MSG);',
 'inverted':'if (EXPR) {} else LexerError(MSG);',
 'unlikely':'if (__builtin_expect(EXPR == 0, 0)) LexerError(MSG);',
 'likely':'if (__builtin_expect(EXPR != 0, 1)) {} else LexerError(MSG);',
 'early-goto':'if (EXPR) goto NEXT; LexerError(MSG); NEXT:;',
 'negative-goto':'if (EXPR == 0) goto FAILURE; goto NEXT; FAILURE: LexerError(MSG); NEXT:;',
 'error-after':'if (EXPR != 0) goto NEXT; LexerError(MSG); NEXT:;',
 'call-branch':'if (!EXPR) { this->LexerError(MSG); }',
 'block-expect':'if (__builtin_expect(!EXPR,0)) { this->LexerError(MSG); }',
}
flags={
 'baseline':['-O2','-frename-registers'],
 'plain':['-O2'],
 'j1':['-O2','-frename-registers','-falign-jumps=1'],
 'j4':['-O2','-frename-registers','-falign-jumps=4'],
 'j8':['-O2','-frename-registers','-falign-jumps=8'],
 'nocross':['-O2','-frename-registers','-fno-crossjumping'],
 'noreorder':['-O2','-frename-registers','-fno-reorder-blocks'],
}
T=Path('/tmp/vuc-binutils215/yycreate-error-guard-oct15f');T.mkdir(parents=True,exist_ok=True)
rows=[];sizes=collections.Counter();best=None
for (fa,sourcea),(fb,sourceb),(flag,opts) in itertools.product(forms.items(),forms.items(),flags.items()):
 if len(rows)>=420:break
 def replace(pattern,expr,key):
  return pattern.replace('EXPR',expr).replace('MSG','"out of dynamic memory in yy_create_buffer()"').replace('NEXT',key+'_end').replace('FAILURE',key+'_failure')
 s1='    '+replace(sourcea,'b','check_1')
 s2='    '+replace(sourceb,'b->buffer','check_2')
 content=src.replace(first,s1).replace(second,s2)
 tag=fa+'-'+fb+'-'+flag
 cpp=T/(tag+'.cpp');obj=T/(tag+'.o');cpp.write_text(content)
 p=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if p.returncode:rows.append(dict(tag=tag,error=p.stderr[-135:]));continue
 try:r=m.run(obj,R/'vuc/config/yy_create_buffer_probe.json')
 except ValueError as e:rows.append(dict(tag=tag,error=str(e)[:160]));continue
 record=dict(tag=tag,length=r['length'],match=r['exact_bytes'],relocations=r['relocations'],literal_bytes=r['verified_literal_bytes'],prefix=next(iter(r['different_offsets']),r['length']))
 rows.append(record);sizes[r['length']]+=1
 if record['length']==139 and record['relocations']==5 and record['literal_bytes']==44 and (best is None or (record['match'],record['prefix'])>(best['match'],best['prefix'])):
  best=record;(T/'best.cpp').write_text(content);(T/'best.o').write_bytes(obj.read_bytes());(T/'best.flags').write_text(' '.join(opts)+'\n')
  if record['match']>131:print('IMPROVEMENT',json.dumps(record),flush=True)
  if record['match']==139:print('EXACT',json.dumps(record),flush=True);break
(T/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
print(json.dumps(dict(attempted=len(rows),strict=sum(x.get('length')==139 and x.get('relocations')==5 and x.get('literal_bytes')==44 for x in rows),sizes=dict(sizes),best=best,top=sorted((x for x in rows if 'match' in x),key=lambda x:(x['match'],x['prefix']),reverse=True)[:10]),indent=2))

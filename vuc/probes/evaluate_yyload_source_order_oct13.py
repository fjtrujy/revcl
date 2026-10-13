#!/usr/bin/env python3
"""Test real flex yy_load_buffer_state independent assignment order, GCC 3.3.3."""
import importlib.util
import itertools
import subprocess
from pathlib import Path
R=Path(__file__).resolve().parents[2]
TMP=Path('/tmp/vuc-binutils215/yyload-order-oct13')
TMP.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
header=(R/'vuc/probes/yy_load_buffer.cpp').read_text()
header=header[:header.index('void yyFlexLexer::yy_load_buffer_state()')]
forms={
 'simple': ['yy_n_chars_ = current_->yy_n_chars;', 'input_ = current_->file;', 'yy_c_buf_p_ = current_->yy_buf_pos;', 'yytext_ptr_ = yy_c_buf_p_;', 'yy_hold_char_ = *yy_c_buf_p_;'],
 'buffer': ['yy_n_chars_ = buf->yy_n_chars;', 'input_ = buf->file;', 'yy_c_buf_p_ = buf->yy_buf_pos;', 'yytext_ptr_ = yy_c_buf_p_;', 'yy_hold_char_ = *yy_c_buf_p_;'],
 'locals': ['yy_n_chars_ = count;', 'input_ = file;', 'yy_c_buf_p_ = pos;', 'yytext_ptr_ = pos;', 'yy_hold_char_ = *pos;'],
 'long-locals': ['yy_n_chars_ = count;', 'input_ = file;', 'yy_c_buf_p_ = pos;', 'yytext_ptr_ = yy_c_buf_p_;', 'yy_hold_char_ = *yytext_ptr_;'],
 'buf-hold': ['yy_n_chars_ = buf->yy_n_chars;', 'input_ = buf->file;', 'yy_c_buf_p_ = buf->yy_buf_pos;', 'yytext_ptr_ = buf->yy_buf_pos;', 'yy_hold_char_ = *buf->yy_buf_pos;'],
}
flags={'r':['-O2','-frename-registers'],'plain':['-O2']}
rank=[]
for name,items in forms.items():
 setup={'simple':'','buffer':'yy_buffer_state *buf = current_;','locals':'yy_buffer_state *buf=current_; int count=buf->yy_n_chars; std::istream *file=buf->file; char *pos=buf->yy_buf_pos;', 'long-locals':'yy_buffer_state *buf=current_; int count=buf->yy_n_chars; std::istream *file=buf->file; char *pos=buf->yy_buf_pos;', 'buf-hold':'yy_buffer_state *buf=current_;'}[name]
 for order in itertools.permutations(range(5)):
  # Preserve the original dependency: write yy_c_buf_p_ before reading it.
  if name in ('simple','buffer') and (order.index(2)>order.index(3) or order.index(2)>order.index(4)):continue
  if name=='long-locals' and order.index(2)>order.index(4):continue
  code=header+'void yyFlexLexer::yy_load_buffer_state(){\n'+setup+'\n'+'\n'.join(items[n] for n in order)+'\n}\n'
  stem=name+'-'+''.join(str(i) for i in order)
  src=TMP/(stem+'.cpp');src.write_text(code)
  for flag,opts in flags.items():
   obj=TMP/(stem+'-'+flag+'.o')
   cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(src),'-o',str(obj)],capture_output=True,text=True)
   if cp.returncode:print('COMPILE_ERROR',stem,flag,cp.stderr[-120:],flush=True);continue
   symbols=subprocess.run(['nm','-S',str(obj)],capture_output=True,text=True)
   found=next((x for x in symbols.stdout.splitlines() if '_ZN11yyFlexLexer20yy_load_buffer_stateEv' in x),'')
   size=int(found.split()[1],16) if found else 0
   if size==39:
    try:
     r=m.run(obj,R/'vuc/config/yy_load_buffer_probe.json')
     rank.append((r['exact_bytes'],stem,flag))
     if r['exact']:
      print('NEW EXACT',stem,flag,src,flush=True)
      raise SystemExit(0)
    except ValueError as e:print('MATCH_ERROR',stem,flag,str(e)[:90],flush=True)
   else:rank.append((0,stem,flag,size))
 print('tested',name,flush=True)
for x in sorted(rank,reverse=True)[:20]:print('BEST',x)

#!/usr/bin/env python3
"""Test genuine lexer-buffer source-unit predecessors for yy_create_buffer()."""
from pathlib import Path
import subprocess,importlib.util,itertools
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/yycreate-real-lexer-oct13');T.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
base=(R/'vuc/probes/yy_create_buffer.cpp').read_text()
lexer=(R/'vuc/src/FlexLexer.cpp').read_text()
head=base[:base.index('yy_buffer_state *yyFlexLexer::yy_create_buffer')]
body=base[base.index('yy_buffer_state *yyFlexLexer::yy_create_buffer'):]
head=head.replace('    void yy_init_buffer(yy_buffer_state *, std::istream *);','''    void yy_init_buffer(yy_buffer_state *, std::istream *);
    void yy_load_buffer_state();
    void yy_switch_to_buffer(yy_buffer_state *);
    void yy_flush_buffer(yy_buffer_state *);
    void yy_delete_buffer(yy_buffer_state *);
    void LexerOutput(const char *, int);
private:
    char *yytext_ptr_;
    char reserved1_[0x18];
    std::istream *input_;
    std::ostream *output_;
    yy_buffer_state *current_;
    char yy_hold_char_;
    char reserved_align_[3];
    int yy_n_chars_;
    char *yy_c_buf_p_;
    char reserved2_[8];
    int yy_did_buffer_switch_on_eof_;
    char reserved3_[0x28];
    unsigned yy_buffer_char_index_;''')
# Genuine yy method definitions from the previous accepted source.
load=lexer[lexer.index('void yyFlexLexer::yy_load_buffer_state()') :].strip()
output=lexer[lexer.index('void yyFlexLexer::LexerOutput('):lexer.index('void yyFlexLexer::LexerError(')].strip()
switch=lexer[lexer.index('void yyFlexLexer::yy_switch_to_buffer('):lexer.index('void yyFlexLexer::yy_init_buffer(')].strip()
other=lexer[lexer.index('void yyFlexLexer::yy_init_buffer('):lexer.index('void yyFlexLexer::yy_delete_buffer(')].strip()
# Map legacy buffer member names to recovered header ones for the adjacent native methods.
head=head.replace('    char *pos;', '    char *yy_buf_pos;').replace('    int size;', '    int reserved_0c;').replace('    int count;', '    int yy_n_chars;').replace('    int own;', '    int yy_is_our_buffer;').replace('    int reserved[4];', '''    int yy_is_interactive_internal;
    int yy_fill_buffer;
    int yy_is_interactive;
    int yy_at_bol;''')
body=body.replace('b->size','b->reserved_0c').replace('b->own','b->yy_is_our_buffer')
parts={'load':load,'output':output,'switch':switch,'other':other}
seqs=[(),('load',),('output',),('switch',),('load','switch'),('switch','load'),('output','load'),('load','output'),('other','load'),('load','other'),('load','switch','other'),('switch','other','load')]
flgs={'r':['-O2','-frename-registers'],'plain':['-O2'],'j1':['-O2','-frename-registers','-falign-jumps=1'], 'j4':['-O2','-frename-registers','-falign-jumps=4']}
rank=[]
for seq in seqs:
 for place in ('before','after'):
  tag='-'.join(seq) or 'isolated'
  name=tag+'-'+place
  content=head+ ('\n'.join(parts[n] for n in seq)+'\n' if place=='before' else '') + body+'\n' + ('\n'.join(parts[n] for n in seq)+'\n' if place=='after' else '')
  src=T/(name+'.cpp');src.write_text(content)
  for flname,flags in flgs.items():
   obj=T/(name+'-'+flname+'.o')
   cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*flags,'-c',str(src),'-o',str(obj)],capture_output=True,text=True)
   if cp.returncode:print('COMPILE_FAIL',name,flname,cp.stderr[-160:],flush=True);continue
   try:
    r=m.run(obj,R/'vuc/config/yy_create_buffer_probe.json')
    rank.append((r['exact_bytes'],name,flname,r['length']))
    if r['exact']:
     print('BYTE EXACT',name,flname,src,flush=True);raise SystemExit(0)
   except ValueError as e:rank.append((0,name,flname,str(e)[:90]))
  print('tested',name,flush=True)
for v in sorted(rank,reverse=True)[:29]:print('BEST',v)

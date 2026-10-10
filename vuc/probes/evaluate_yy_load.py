#!/usr/bin/env python3
"""Source organization probes for original yy_load_buffer_state without asm."""
from pathlib import Path
import importlib.util,subprocess
ROOT=Path(__file__).resolve().parents[2]
TMP=Path("/tmp/vuc-binutils215")
spec=importlib.util.spec_from_file_location("m",ROOT/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
raw=(ROOT/"vuc/probes/yy_load_buffer.cpp").read_text()
needle="""    yy_n_chars_ = current_->yy_n_chars;
    input_ = current_->file;
    yy_c_buf_p_ = current_->yy_buf_pos;
    yytext_ptr_ = yy_c_buf_p_;
    yy_hold_char_ = *yy_c_buf_p_;"""
assert needle in raw
variants={
"baseline":needle,
"state-local":"""    yy_buffer_state *state=current_;
    yy_n_chars_ = state->yy_n_chars;
    input_ = state->file;
    yy_c_buf_p_ = state->yy_buf_pos;
    yytext_ptr_ = yy_c_buf_p_;
    yy_hold_char_ = *yy_c_buf_p_;""",
"load-before-store":"""    yy_buffer_state *state=current_;
    int count = state->yy_n_chars;
    std::istream *input=state->file;
    char *pos=state->yy_buf_pos;
    yy_n_chars_ = count;
    input_ = input;
    yy_c_buf_p_ = pos;
    yytext_ptr_ = pos;
    yy_hold_char_ = *pos;""",
"count-and-file":"""    yy_buffer_state *state=current_;
    int count=state->yy_n_chars;
    std::istream *input=state->file;
    yy_n_chars_=count;
    input_=input;
    char *pos=state->yy_buf_pos;
    yy_c_buf_p_=pos;
    yytext_ptr_=pos;
    yy_hold_char_=*pos;""",
"field-order":"""    yy_buffer_state *state=current_;
    int count=state->yy_n_chars;
    std::istream *input=state->file;
    yy_n_chars_=count;
    char *pos=state->yy_buf_pos;
    input_=input;
    yy_c_buf_p_=pos;
    yytext_ptr_=pos;
    yy_hold_char_=*pos;""",
"direct-state":"""    yy_buffer_state *state=current_;
    yy_n_chars_=state->yy_n_chars;
    input_=state->file;
    yy_c_buf_p_=state->yy_buf_pos;
    yytext_ptr_=state->yy_buf_pos;
    yy_hold_char_=*state->yy_buf_pos;""",
"curr-then-ptr":"""    yy_buffer_state *state=current_;
    int count=state->yy_n_chars;
    yy_n_chars_=count;
    input_=state->file;
    char *pos=state->yy_buf_pos;
    yy_c_buf_p_=pos;
    yytext_ptr_=pos;
    yy_hold_char_=*pos;""",
"pointer-local":"""    yy_buffer_state *state=current_;
    int count=state->yy_n_chars;
    char *position=state->yy_buf_pos;
    input_=state->file;
    yy_n_chars_=count;
    yy_c_buf_p_=position;
    yytext_ptr_=position;
    yy_hold_char_=*position;""",
}
out=[]
for label,body in variants.items():
 p=TMP/("load-variant-"+label+".cpp");p.write_text(raw.replace(needle,body))
 for flagsname,flags in [("ren",["-O2","-frename-registers"]),("ren-j1",["-O2","-frename-registers","-falign-jumps=1"])]:
  obj=TMP/("load-variant-"+label+"-"+flagsname+".o")
  cp=subprocess.run([str(TMP/"g++-jan25-as215"),*flags,"-c",str(p),"-o",str(obj)],capture_output=True,text=True)
  if cp.returncode:out.append((0,label,flagsname,cp.stderr[-80:]));continue
  try:
   r=m.run(obj,ROOT/"vuc/config/yy_load_buffer_probe.json")
   out.append((r["exact_bytes"],label,flagsname,str(r["length"])))
  except Exception as e:out.append((0,label,flagsname,str(e)[:70]))
out.sort(reverse=True)
for x in out:print(*x)

#!/usr/bin/env python3
"""Candidate source-form investigation for the original Flex buffer allocator."""
from pathlib import Path
import importlib.util, subprocess
ROOT=Path(__file__).resolve().parents[2]
TMP=Path("/tmp/vuc-binutils215")
sp=importlib.util.spec_from_file_location("vucmatch",ROOT/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
source=(ROOT/"vuc/probes/yy_create_buffer.cpp").read_text()
fatal='    if (!b)\n        LexerError("out of dynamic memory in yy_create_buffer()");'
after='    if (!b->buffer)\n        LexerError("out of dynamic memory in yy_create_buffer()");'
assert fatal in source and after in source
message='"out of dynamic memory in yy_create_buffer()"'
forms={
 "original":(fatal,after),
 "locals":(f"    if (!b) {{ const char *message={message}; LexerError(message); }}", f"    if (!b->buffer) {{ const char *message={message}; LexerError(message); }}"),
 "this-lexer":(fatal.replace("LexerError(","this->LexerError("),after.replace("LexerError(","this->LexerError(")),
 "null-compare":(fatal.replace("!b","b == 0"),after.replace("!b->buffer","b->buffer == 0")),
 "negation":(fatal.replace("!b","!(b != 0)"),after.replace("!b->buffer","!(b->buffer != 0)")),
 "tmp-message":(f"    const char *message={message};\n    if (!b) LexerError(message);", f"    const char *message2={message};\n    if (!b->buffer) LexerError(message2);"),
 "fatal-object":(f"    if (!b) {{ yyFlexLexer *self=this; self->LexerError({message}); }}", f"    if (!b->buffer) {{ yyFlexLexer *self=this; self->LexerError({message}); }}"),
 "fatal-callblock":(f"    if (!b) {{ this->LexerError({message}); }}", f"    if (!b->buffer) {{ this->LexerError({message}); }}"),
 "inverted":(f"    if (b) {{ }} else {{ LexerError({message}); }}", f"    if (b->buffer) {{ }} else {{ LexerError({message}); }}"),
 "volatile-msg":(f"    if (!b) {{ const char * volatile msg={message}; LexerError(msg); }}", f"    if (!b->buffer) {{ const char * volatile msg={message}; LexerError(msg); }}"),
}
fl={
 "ren":["-O2","-frename-registers"],
 "plain":["-O2"],
 "ren-j1":["-O2","-frename-registers","-falign-jumps=1"],
 "ren-no-cse":["-O2","-frename-registers","-fno-cse-follow-jumps"],
 "ren-no-regmove":["-O2","-frename-registers","-fno-regmove"],
}
scores=[]
for name,(first,second) in forms.items():
 p=TMP/("yycreate-"+name+".cpp");p.write_text(source.replace(fatal,first).replace(after,second))
 for optsname,opts in fl.items():
  obj=TMP/("yycreate-"+name+"-"+optsname+".o")
  cp=subprocess.run([str(TMP/"g++-jan25-as215"),*opts,"-c",str(p),"-o",str(obj)],capture_output=True,text=True)
  if cp.returncode:scores.append((0,name,optsname,cp.stderr[-80:]));continue
  try:
   match=m.run(obj,ROOT/"vuc/config/yy_create_buffer_probe.json")
   scores.append((match["exact_bytes"],name,optsname,str(match["length"])))
  except Exception as e:scores.append((0,name,optsname,str(e)[:85]))
scores.sort(reverse=True)
for record in scores[:30]:print(*record)

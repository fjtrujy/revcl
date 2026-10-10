#!/usr/bin/env python3
"""Natural source variants for VUC yy_delete_buffer register assignment."""
from pathlib import Path
import importlib.util,subprocess
ROOT=Path(__file__).resolve().parents[2];TMP=Path("/tmp/vuc-binutils215")
sp=importlib.util.spec_from_file_location("match",ROOT/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
raw=(ROOT/"vuc/probes/yy_delete_buffer.cpp").read_text()
original="    if (buffer->yy_is_our_buffer)\n        std::free(buffer->yy_ch_buf);"
assert original in raw
variants={
 "original":original,
 "flag-local":"    int own=buffer->yy_is_our_buffer;\n    if (own)\n        std::free(buffer->yy_ch_buf);",
 "flag-unsigned":"    unsigned own=buffer->yy_is_our_buffer;\n    if (own)\n        std::free(buffer->yy_ch_buf);",
 "flag-bool":"    bool own=buffer->yy_is_our_buffer;\n    if (own)\n        std::free(buffer->yy_ch_buf);",
 "flag-pointer":"    int *own=&buffer->yy_is_our_buffer;\n    if (*own)\n        std::free(buffer->yy_ch_buf);",
 "flag-not":"    if (!buffer->yy_is_our_buffer) {} else\n        std::free(buffer->yy_ch_buf);",
 "flag-true":"    if (buffer->yy_is_our_buffer != 0)\n        std::free(buffer->yy_ch_buf);",
 "flag-cond":"    if (buffer->yy_is_our_buffer == 0) {} else\n        std::free(buffer->yy_ch_buf);",
 "flag-and":"    if (buffer->yy_is_our_buffer && buffer->yy_ch_buf)\n        std::free(buffer->yy_ch_buf);",
 "flag-conditional":"    if (buffer->yy_is_our_buffer) { char *p=buffer->yy_ch_buf; std::free(p); }",
}
fl={
 "ren":["-O2","-frename-registers"],
 "plain":["-O2"],
 "ren-nocse":["-O2","-frename-registers","-fno-cse-follow-jumps"],
 "ren-nogcse":["-O2","-frename-registers","-fno-gcse"],
 "ren-noregmove":["-O2","-frename-registers","-fno-regmove"],
 "ren-nostrict":["-O2","-frename-registers","-fno-strict-aliasing"],
}
scores=[]
for name,body in variants.items():
 p=TMP/("flex-delete-"+name+".cpp")
 p.write_text(raw.replace(original,body))
 for label,opts in fl.items():
  out=TMP/("flex-delete-"+name+"-"+label+".o")
  cp=subprocess.run([str(TMP/"g++-jan25-as215"),*opts,"-c",str(p),"-o",str(out)],capture_output=True,text=True)
  if cp.returncode:scores.append((0,name,label,cp.stderr[-80:]));continue
  try:
   r=m.run(out,ROOT/"vuc/config/yy_delete_buffer_probe.json")
   scores.append((r["exact_bytes"],name,label,str(r["length"])))
  except Exception as e:scores.append((0,name,label,str(e)[:65]))
scores.sort(reverse=True)
for row in scores[:25]:print(*row)

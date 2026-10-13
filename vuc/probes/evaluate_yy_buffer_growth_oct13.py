#!/usr/bin/env python3
"""Natural C++ Flex buffer-growth source forms under both period GCCs.

The baseline reconstructs scanner buffer movement, EOF, virtual input, and
realloc without source-level machine-code forcing. Measure authentic buffer
ownership / allocation expressions and original-size effects; no source changes
outside the sandboxed Flex probe.
"""
from pathlib import Path
from itertools import product
import subprocess
ROOT=Path(__file__).resolve().parents[2]
TMP=Path('/tmp/vuc-binutils215/yy-buffer-growth-oct13')
TMP.mkdir(parents=True,exist_ok=True)
base=(ROOT/'vuc/probes/yy_get_next_buffer_oct13.cpp').read_text()
start=base.index('            if (b->yy_is_our_buffer)')
end=base.index('            if (!b->yy_ch_buf)',start)
pre=base[:start]; post=base[end:]
candidates={
"baseline":base[start:end],
"split-memory":"""            if (b->yy_is_our_buffer) {
                int new_size = b->yy_buf_size * 2;
                if (new_size <= 0)
                    b->yy_buf_size += ((unsigned)b->yy_buf_size >> 3);
                else
                    b->yy_buf_size *= 2;
                b->yy_ch_buf = (char*) std::realloc(b->yy_ch_buf, b->yy_buf_size + 2);
            } else
                b->yy_ch_buf = 0;
""",
"split-assign":"""            if (b->yy_is_our_buffer) {
                int new_size = b->yy_buf_size * 2;
                if (new_size <= 0)
                    b->yy_buf_size = b->yy_buf_size + ((unsigned)b->yy_buf_size >> 3);
                else
                    b->yy_buf_size = new_size;
                b->yy_ch_buf = (char*) std::realloc(b->yy_ch_buf, b->yy_buf_size + 2);
            } else
                b->yy_ch_buf = 0;
""",
"unsigned-fallback":"""            if (b->yy_is_our_buffer) {
                int new_size = b->yy_buf_size * 2;
                if (new_size <= 0)
                    new_size = b->yy_buf_size + ((unsigned)b->yy_buf_size >> 3);
                b->yy_buf_size = new_size;
                b->yy_ch_buf = (char*) std::realloc(b->yy_ch_buf, b->yy_buf_size + 2);
            } else
                b->yy_ch_buf = 0;
""",
"compound-size":"""            if (b->yy_is_our_buffer) {
                if (2 * b->yy_buf_size <= 0)
                    b->yy_buf_size += ((unsigned)b->yy_buf_size >> 3);
                else
                    b->yy_buf_size *= 2;
                b->yy_ch_buf = (char*) std::realloc(b->yy_ch_buf, b->yy_buf_size + 2);
            } else
                b->yy_ch_buf = 0;
""",
"saved-old-pointer":"""            if (b->yy_is_our_buffer) {
                char *old_buf = b->yy_ch_buf;
                int new_size = b->yy_buf_size * 2;
                if (new_size <= 0)
                    new_size = b->yy_buf_size + ((unsigned)b->yy_buf_size >> 3);
                b->yy_buf_size = new_size;
                b->yy_ch_buf = (char*) std::realloc(old_buf, b->yy_buf_size + 2);
            } else
                b->yy_ch_buf = 0;
""",
"saved-old-and-compound":"""            if (b->yy_is_our_buffer) {
                char *old_buf = b->yy_ch_buf;
                if (2 * b->yy_buf_size <= 0)
                    b->yy_buf_size += ((unsigned)b->yy_buf_size >> 3);
                else
                    b->yy_buf_size *= 2;
                b->yy_ch_buf = (char*) std::realloc(old_buf, b->yy_buf_size + 2);
            } else
                b->yy_ch_buf = 0;
""",
"local-result":"""            if (b->yy_is_our_buffer) {
                int new_size = b->yy_buf_size * 2;
                if (new_size <= 0)
                    new_size = b->yy_buf_size + ((unsigned)b->yy_buf_size >> 3);
                b->yy_buf_size = new_size;
                char *new_buf = (char*) std::realloc(b->yy_ch_buf, b->yy_buf_size + 2);
                b->yy_ch_buf = new_buf;
            } else
                b->yy_ch_buf = 0;
""",
"unsigned-shift-else-first":"""            if (b->yy_is_our_buffer) {
                int proposed_size = 2 * b->yy_buf_size;
                if (proposed_size > 0)
                    b->yy_buf_size = proposed_size;
                else
                    b->yy_buf_size += ((unsigned)b->yy_buf_size >> 3);
                b->yy_ch_buf = (char*) std::realloc(b->yy_ch_buf, b->yy_buf_size + 2);
            } else
                b->yy_ch_buf = 0;
"""
}
flags={
"ren":["-O2","-frename-registers"],
"j1":["-O2","-frename-registers","-falign-jumps=1"],
"j4":["-O2","-frename-registers","-falign-jumps=4"],
"no-ren":["-O2"],
}
results=[]
for label, growth in candidates.items():
 src=TMP/(label+".cpp");src.write_text(pre+growth+post)
 for fl,opts in flags.items():
  obj=TMP/(label+"-"+fl+".o")
  build=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*opts,"-c",str(src),"-o",str(obj)],capture_output=True,text=True)
  if build.returncode:
   print('ERROR',label,fl,build.stderr[-160:],flush=True);continue
  nm=subprocess.check_output(["nm","-S",str(obj)],text=True)
  symbol=next((x for x in nm.splitlines() if x.endswith("_ZN11yyFlexLexer18yy_get_next_bufferEv")),"")
  size=int(symbol.split()[1],16) if symbol else -1
  results.append((abs(size-550),size,label,fl))
 print('DONE',label,flush=True)
print('NEAREST')
for row in sorted(results)[:42]:print(row)

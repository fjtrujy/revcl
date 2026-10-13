#!/usr/bin/env python3
"""Genuine yy_create_buffer source/local lifetime alternatives for GCC 3.3.3."""
from pathlib import Path
import importlib.util, subprocess
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/yycreate-receivers-oct13');T.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
source=(R/'vuc/probes/yy_create_buffer.cpp').read_text()
base='''    yy_buffer_state *b = (yy_buffer_state *)std::malloc(0x28);
    if (!b)
        LexerError("out of dynamic memory in yy_create_buffer()");
    b->size = size;
    b->buffer = (char *)std::malloc(size + 2);
    if (!b->buffer)
        LexerError("out of dynamic memory in yy_create_buffer()");
    b->own = 1;
    yy_init_buffer(b, file);
    return b;'''
assert base in source
v={
 'original':base,
 'buffer-local':base.replace('    b->buffer = (char *)std::malloc(size + 2);','    char *allocated = (char *)std::malloc(size + 2);\n    b->buffer = allocated;'),
 'buffer-local-test':base.replace('    b->buffer = (char *)std::malloc(size + 2);\n    if (!b->buffer)','    char *allocated = (char *)std::malloc(size + 2);\n    b->buffer = allocated;\n    if (!allocated)'),
 'int-bufferlen':base.replace('    b->buffer = (char *)std::malloc(size + 2);','    int allocationSize = size + 2;\n    b->buffer = (char *)std::malloc(allocationSize);'),
 'unsigned-bufferlen':base.replace('    b->buffer = (char *)std::malloc(size + 2);','    unsigned allocationSize = (unsigned)size + 2;\n    b->buffer = (char *)std::malloc(allocationSize);'),
 'state-alias':base.replace('    b->size = size;', '    yy_buffer_state *state = b;\n    state->size = size;').replace('    b->buffer', '    state->buffer').replace('    if (!b->buffer)','    if (!state->buffer)').replace('    b->own = 1;', '    state->own = 1;').replace('yy_init_buffer(b, file)', 'yy_init_buffer(state, file)').replace('return b;', 'return state;'),
 'this-alias':base.replace('    yy_buffer_state *b =', '    yyFlexLexer *self=this;\n    yy_buffer_state *b =').replace('        LexerError(', '        self->LexerError(').replace('    yy_init_buffer(b, file);','    self->yy_init_buffer(b, file);'),
 'file-alias':base.replace('    yy_buffer_state *b =','    std::istream *stream=file;\n    yy_buffer_state *b =').replace('yy_init_buffer(b, file)','yy_init_buffer(b, stream)'),
 'message-alias':base.replace('    yy_buffer_state *b =','    const char *message="out of dynamic memory in yy_create_buffer()";\n    yy_buffer_state *b =').replace('LexerError("out of dynamic memory in yy_create_buffer()")','LexerError(message)'),
 'message-after':base.replace('    b->size = size;', '    const char *message="out of dynamic memory in yy_create_buffer()";\n    b->size = size;').replace('    if (!b->buffer)\n        LexerError("out of dynamic memory in yy_create_buffer()");','    if (!b->buffer)\n        LexerError(message);'),
 'null-local':base.replace('    if (!b->buffer)\n        LexerError("out of dynamic memory in yy_create_buffer()");', '    char *buffer = b->buffer;\n    if (!buffer) LexerError("out of dynamic memory in yy_create_buffer()");'),
 'init-alias':base.replace('    b->own = 1;', '    yy_buffer_state *state = b;\n    state->own = 1;').replace('yy_init_buffer(b, file)', 'yy_init_buffer(state, file)'),
 'size-alias':base.replace('    b->size = size;', '    int bufferSize=size;\n    b->size = bufferSize;').replace('std::malloc(size + 2)', 'std::malloc(bufferSize + 2)'),
 'inline-check':base.replace('    if (!b)\n        LexerError("out of dynamic memory in yy_create_buffer()");', '    if (!b) { this->LexerError("out of dynamic memory in yy_create_buffer()"); }').replace('    if (!b->buffer)\n        LexerError("out of dynamic memory in yy_create_buffer()");', '    if (!b->buffer) { this->LexerError("out of dynamic memory in yy_create_buffer()"); }'),
 'explicit-malloc':base.replace('std::malloc','::malloc'),
 'error-alias-object':base.replace('    yy_buffer_state *b =','    yyFlexLexer &self = *this;\n    yy_buffer_state *b =').replace('        LexerError(', '        self.LexerError('),
 'size-store-after-buffer':base.replace('    b->size = size;\n','').replace('    b->buffer = (char *)std::malloc(size + 2);','    b->buffer = (char *)std::malloc(size + 2);\n    b->size = size;'),
}
flags={'ren':['-O2','-frename-registers'],'plain':['-O2'],'j1':['-O2','-frename-registers','-falign-jumps=1'],'j4':['-O2','-frename-registers','-falign-jumps=4']}
rank=[]
for name,body in v.items():
 path=T/(name+'.cpp');path.write_text(source.replace(base,body))
 for fl,opts in flags.items():
  obj=T/(name+'-'+fl+'.o')
  cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(path),'-o',str(obj)],capture_output=True,text=True)
  if cp.returncode:print(name,fl,'COMPILE_FAIL',cp.stderr[-130:],flush=True);continue
  try:
   score=m.run(obj,R/'vuc/config/yy_create_buffer_probe.json')
   rank.append((score['exact_bytes'],name,fl,score['length']))
   if score['exact']:print('BYTE-EXACT',name,fl,path,flush=True);raise SystemExit(0)
  except ValueError as e:rank.append((0,name,fl,str(e)[:100]))
 print('checked',name,flush=True)
for x in sorted(rank,reverse=True)[:35]:print('BEST',x)

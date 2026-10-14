#!/usr/bin/env python3
"""Find natural C++03 source-lifetime explanation of 30-byte VUC prologue.

Original ELF loads literal into EAX before the receiver into EDX.
Natural source forms, real iostream neighbours, GCC 3.3.3 optimizer flags,
and original 92-byte string literal are checked; no inline asm or NOPs.
"""
import importlib.util,json,subprocess,itertools
from pathlib import Path
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/prologue-source-oct14')
T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
full=(R/'vuc/src/AsmOutputUnit.cpp').read_text()
start=full.index('void AsmOutput_WritePrologue(')
end=full.index('void AsmOutput_WriteEpilogue(',start)
function=full[start:end]
prefix=full[:start]
literal=function[function.index('out <<')+len('out <<'):function.rindex(';')].strip()
original_config=json.loads((R/'vuc/config/asm_output_unit_oct13_probe.json').read_text())
original_config['relocation_literals']={'.rodata':[{'offset':0,'size':92,'address':'0x080720a0'}]}
configs=[]
# A bounded, independently scored control set. Real multi-function source
# context is covered by evaluate_asm_real_unit_neighbors_oct13.py instead.
for style in ('isolated',):
 forms={
 'direct':'out << LIT;',
 'std-call':'std::operator<<(out, LIT);',
 'ostream-ref':'std::ostream &stream = out; stream << LIT;',
 'ostream-call':'std::ostream &stream = out; std::operator<<(stream, LIT);',
 'literal-local':'const char *text = LIT; out << text;',
 'literal-auto-ref':'const char *const &text = LIT; out << text;',
 'literal-ref':'const char *const text = LIT; out << text;',
 'literal-pointer':'const char *text = LIT; std::operator<<(out, text);',
 'ostream-pointer':'std::ofstream *stream = &out; (*stream) << LIT;',
 'literal-first-arg':'const char *text = LIT; std::ofstream &stream=out; stream << text;',
 'receiver-first-arg':'std::ofstream &stream=out; const char *text=LIT; stream << text;',
 'literal-named':'const char *const text=LIT; std::ostream &stream=out; stream << text;',
 'explicit-call':'operator<<(out, LIT);',
 'static-literal':'static const char text[] = LIT; out << text;',
 'literal-pointer-ref':'const char *ptr = LIT; const char *const &text=ptr; out << text;',
 'ostream-ref-second':'const char *text=LIT; std::ostream &ref = out; std::operator<<(ref,text);',
 'void-ostream-cast':'std::ostream &ref=static_cast<std::ostream &>(out); ref << LIT;',
 'const-arg-local':'const char *const text = LIT; out << text;',
 'function-call-return':'(out << LIT);',
 'local-receiver':'std::ofstream &output=out; output << LIT;',
 'source-word':'const char *literal=LIT; (out << literal);',
 }
 for name,body in list(forms.items())[:12]:
  src=prefix+function[:function.index('{')+1]+'\n    '+body.replace('LIT',literal)+'\n}\n'
  if style=='with-epilogue':src+=full[end:full.index('#include <cstring>',end)]
  if style=='with-asm-unit':src+=full[end:]
  configs.append((style,name,src))
flags={
 'r':['-O2','-frename-registers'],
 'nr':['-O2'],
 'rj1':['-O2','-frename-registers','-falign-jumps=1'],
 'rj4':['-O2','-frename-registers','-falign-jumps=4'],
 'rj8':['-O2','-frename-registers','-falign-jumps=8'],
 'rno_sched':['-O2','-frename-registers','-fno-schedule-insns2'],
 'rno_reg':['-O2','-frename-registers','-fno-regmove'],
 'rno_gcse':['-O2','-frename-registers','-fno-gcse'],
 'rno_cprop':['-O2','-frename-registers','-fno-cprop-registers'],
 'rno_reorder':['-O2','-frename-registers','-fno-reorder-blocks'],
 'rj2':['-O2','-frename-registers','-falign-jumps=2'],
 'rnolabel':['-O2','-frename-registers','-falign-labels=1'],
}
scores=[]
for style,name,body in configs:
 source=T/(style+'-'+name+'.cpp');source.write_text(body)
 for label,opts in flags.items():
  obj=T/(style+'-'+name+'-'+label+'.o')
  c=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(source),'-o',str(obj)],capture_output=True,text=True)
  if c.returncode:
   print('COMPILE',style,name,label,c.stderr[-120:],flush=True);continue
  config=dict(original_config)
  config['source']='vuc/probes/asm_output_hypothesis.cpp'
  config['flags']=opts
  if style!='isolated':
   # Later literal bytes must be verified, not silently ignored.
   config['relocation_literals']={'.rodata':[{'offset':0,'size':92,'address':'0x080720a0'},
      {'offset':92,'size':19,'address':'0x080721c4'}]}
  cfg=T/(style+'-'+name+'-'+label+'.json')
  cfg.write_text(json.dumps(config))
  try:
   score=m.run(obj,cfg)
   scores.append((score['exact_bytes'],score['length'],style,name,label,score['different_offsets'][:10]))
   if score['exact']:
    print('EXACT',style,name,label,str(source),flush=True)
    raise SystemExit(0)
  except ValueError as e:
   scores.append((0,-1,style,name,label,str(e)[:80]))
 print('DONE',style,name,flush=True)
print('BEST')
for row in sorted(scores,reverse=True)[:30]:print(row,flush=True)

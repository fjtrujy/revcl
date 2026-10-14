#!/usr/bin/env python3
"""Investigate remaining 84-byte VUC glibc crt __libc_csu_init.

GCC 3.3.3 2003-12/2004-01 historical C-only startup source and PIC
relocations. All proposals use real init/fini arrays; selected exact
LibcCsu.c is never changed. Only original-length complete relocation-
authenticated functions are possible exact/experimental candidates.
"""
from pathlib import Path
import subprocess,collections,importlib.util,itertools
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/csu-init-source-ownership-oct14');T.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
src=(R/'vuc/probes/libc_csu_hidden.c').read_text()
head=src[:src.index('void __libc_csu_init(')]
fini=src[src.index('void __libc_csu_fini('):]
forms={
 'simple':'''_init(); unsigned n=__init_array_end-__init_array_start; unsigned i=0; for(;i<n;++i)(*__init_array_start[i])();''',
 'zero-before-init':'''unsigned i=0; _init(); unsigned n=__init_array_end-__init_array_start; for(;i<n;++i)(*__init_array_start[i])();''',
 'size-before-init':'''unsigned n=__init_array_end-__init_array_start;unsigned i=0;_init();for(;i<n;++i)(*__init_array_start[i])();''',
 'init-postincrement':'''unsigned i=0;_init();unsigned n=__init_array_end-__init_array_start;while(i<n)(*__init_array_start[i++])();''',
 'init-before-test':'''unsigned i=0; _init();unsigned n=__init_array_end-__init_array_start;if(i<n) do {(*__init_array_start[i])();++i;}while(i<n);''',
 'init-after-test':'''unsigned i=0; _init();unsigned n=__init_array_end-__init_array_start;if(n>i) do {(*__init_array_start[i])();++i;}while(i<n);''',
 'loop-down':'''unsigned i=0;_init();unsigned n=__init_array_end-__init_array_start; while(i++<n) (*__init_array_start[i-1])();''',
 'cache-count':'''unsigned i=0;_init();const unsigned n=__init_array_end-__init_array_start; while(i<n){(*__init_array_start[i])();++i;}''',
 'cache-base':'''unsigned i=0;_init();startup_hook* base=__init_array_start;unsigned n=__init_array_end-__init_array_start;while(i<n){(*base[i])();++i;}''',
 'ptr-register':'''unsigned i=0;_init();unsigned n=__init_array_end-__init_array_start;for(;i<n;++i){startup_hook *p=__init_array_start+i;(*p)();}''',
 'pointer-advance':'''startup_hook* p=__init_array_start;_init();while(p<__init_array_end)(*p++)();''',
 'index-register':'''register unsigned i=0; _init();unsigned n=__init_array_end-__init_array_start; for(;i<n;++i)(*__init_array_start[i])();''',
 'signed-index':'''int i=0; _init();int n=__init_array_end-__init_array_start;for(;i<n;++i)(*__init_array_start[i])();''',
 'switch-break':'''unsigned i=0;_init();unsigned n=__init_array_end-__init_array_start;for(;;){if(i>=n)break; (*__init_array_start[i++])();}''',
 'size-volatile':'''volatile unsigned i=0;_init();unsigned n=__init_array_end-__init_array_start;while(i<n){(*__init_array_start[i])(); ++i;}''',
}
opts={
 'j1':['-xc','-O2','-fPIC','-falign-jumps=1'],
 'j2':['-xc','-O2','-fPIC','-falign-jumps=2'],
 'j4':['-xc','-O2','-fPIC','-falign-jumps=4'],
 'j8':['-xc','-O2','-fPIC','-falign-jumps=8'],
 'ren-j1':['-xc','-O2','-fPIC','-frename-registers','-falign-jumps=1'],
 'ren-j4':['-xc','-O2','-fPIC','-frename-registers','-falign-jumps=4'],
 'j1-loop16':['-xc','-O2','-fPIC','-falign-jumps=1','-falign-loops=16'],
 'j1-loop8':['-xc','-O2','-fPIC','-falign-jumps=1','-falign-loops=8'],
 'j1-label4':['-xc','-O2','-fPIC','-falign-jumps=1','-falign-labels=4'],
 'no-reorder':['-xc','-O2','-fPIC','-fno-reorder-blocks','-falign-jumps=1'],
 'no-regmove':['-xc','-O2','-fPIC','-fno-regmove','-falign-jumps=1'],
 'no-rename':['-xc','-O2','-fPIC','-fno-rename-registers','-falign-jumps=1'],
}
compilers={'jan25':'/tmp/vuc-binutils215/g++-jan25-as215',
'dec06':'/tmp/vuc-binutils215/g++-dec06-as215'}
rows=[];sizes=collections.Counter();attempts=0
for name,body in forms.items():
 cpp=T/(name+'.c')
 cpp.write_text(head+'void __libc_csu_init(int argc,char **argv,char **envp)\n{\n'+body+'\n}\n'+fini)
 for snap,compiler in compilers.items():
  for label,flags in opts.items():
   obj=T/(name+'-'+snap+'-'+label+'.o')
   proc=subprocess.run([compiler,*flags,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
   attempts+=1
   if proc.returncode:
    print('ERROR',name,snap,label,proc.stderr[-90:],flush=True);continue
   sym=next((a for a in subprocess.check_output(['nm','-S',str(obj)],text=True).splitlines() if a.split()[-1:]==['__libc_csu_init']),'')
   size=int(sym.split()[1],16) if sym else -1;sizes[size]+=1
   if size!=84:continue
   try:
    result=m.run(obj,R/'vuc/config/libc_csu_init_pic_probe.json')
    rows.append((result['exact_bytes'],name,snap,label,result['different_offsets'][:14],str(obj)))
    if result['exact']:
     print('NEW_EXACT',rows[-1],flush=True);raise SystemExit(0)
   except ValueError as e:print('REJECT',name,snap,label,str(e)[:110],flush=True)
 print('DONE',name,'ATTEMPTS',attempts,'BEST',max((a[0] for a in rows),default=0),flush=True)
print('TOTAL',attempts,'LENGTHS',sizes.most_common(20),'MATCHES',len(rows),flush=True)
print('TOP')
for row in sorted(rows,reverse=True)[:32]:print(row,flush=True)

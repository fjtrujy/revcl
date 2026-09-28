.init_vf_all
.init_vi_all
.syntax new
.vu
--enter
--endenter
iaddiu count, vi00, 2
loop:
iaddi count, count, -1
ibne count, vi00, loop
isw.x count, 0(vi00)
--exit
--endexit

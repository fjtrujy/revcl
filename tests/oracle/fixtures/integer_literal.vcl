.init_vf_all
.init_vi_all
.syntax new
.vu
--enter
--endenter
iaddiu address, vi00, 4
isw.x address, 0(vi00)
--exit
--endexit
.END

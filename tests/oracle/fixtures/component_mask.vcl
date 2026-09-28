.init_vf_all
.init_vi_all
.syntax new
.vu
--enter
--endenter
lq value, 0(vi00)
add.xyz value, value, vf00
sq value, 1(vi00)
--exit
--endexit
.END

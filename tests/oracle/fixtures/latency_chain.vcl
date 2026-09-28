.init_vf_all
.init_vi_all
.syntax new
.vu
--enter
--endenter
lq value, 0(vi00)
mul doubled, value, value
add result, doubled, value
sq result, 1(vi00)
--exit
--endexit
.END

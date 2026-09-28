.init_vf_all
.init_vi_all
.syntax new
.vu
--enter
--endenter
add value, vf00, vf00
iaddiu address, vi00, 4
sq value, 0(address)
--exit
--endexit
.END

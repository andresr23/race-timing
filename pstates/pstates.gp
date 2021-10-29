set terminal png font 'Helvetica, 19' size 960,540 enhanced
if(!exists("platform")) platform = "skylake"
set output 'pstates-'.platform.'.png'

unset key
set grid

set ylabel 'Hit-{/Symbol D}_{TSC}' offset 1.9
set xlabel 'Probe round' offset 0,0.1

set xrange [0:100000]
set xtics 0,10000,90000 font ',13'

set yrange [0:400]
set ytics 0,100,300 font ',13'

plot 'pstates-'.platform.'.dat' u:1 with points ps 0 lc 'blue'

set terminal png font 'Helvetica, 19' size 960,1080 enhanced
if(!exists("platform")) platform = "skylake"
set output 'explore-sc-'.platform.'.png'

unset key

_x = 400
_y = 30

set palette rgb 30,31,32

set xrange[0.5:_x + 0.5]
set xtics 0,50,400 font ',13'
set xlabel 'OD\_n' font ',13'

set yrange[0.5:_y + 0.5]
set ytics 0,5,30 font ',13'
set ylabel 'Race Interim' font ',13'

set multiplot layout 2,1

## Metric of Observability Matrix
set cbrange[0.5:1]
set cbtics 0.5,0.1,1 font ',13'
plot 'explore-sc-acc-'.platform.'.dat' matrix using (1+$1):(1+$2):3 with image

## Assertion count Matrix
set cbrange[0:1000]
set cbtics 0,200,1000 font ',13'
plot 'explore-sc-pos-'.platform.'.dat' matrix using (1+$1):(1+$2):3 with image

unset multiplot

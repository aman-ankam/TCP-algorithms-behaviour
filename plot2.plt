set terminal png
set output "plot2.png"
set title "Packets Dropped"
set xlabel "Time"
set ylabel "No. of packets Dropped"
plot "TcpWestwood_dropped" using 1:2 with lines title "WestWood",\
	"TcpScalable_dropped" using 1:2 with lines title "Scalable",\
	"TcpNewReno_dropped" using 1:2 with lines title "NewReno",\
	"TcpVegas_dropped" using 1:2 with lines title "Vegas",\
	"TcpHybla_dropped" using 1:2 with lines title "Hybla"

set terminal png
set output "plot3.png"
set title "Cumulative Bytes Transferred"
set xlabel "Time"
set ylabel "Cumulative Bytes Transferred"
plot "TcpWestwood_total" using 1:2 with lines title "WestWood",\
	"TcpScalable_total" using 1:2 with lines title "Scalable",\
	"TcpNewReno_total" using 1:2 with lines title "NewReno",\
	"TcpVegas_total" using 1:2 with lines title "Vegas",\
	"TcpHybla_total" using 1:2 with lines title "Hybla"

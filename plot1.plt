set terminal png
set output "plot1.png"
set title "Congestion Window"
set xlabel "Time"
set ylabel "Congestion Window size"
plot "TcpWestwood_cwnd" using 1:2 with lines title "WestWood",\
	"TcpScalable_cwnd" using 1:2 with lines title "Scalable",\
	"TcpNewReno_cwnd" using 1:2 with lines title "NewReno",\
	"TcpVegas_cwnd" using 1:2 with lines title "Vegas",\
	"TcpHybla_cwnd" using 1:2 with lines title "Hybla"

./waf --run "code --prot=TcpNewReno"
./waf --run "code --prot=TcpHybla"
./waf --run "code --prot=TcpWestwood"
./waf --run "code --prot=TcpScalable"
./waf --run "code --prot=TcpVegas"

gnuplot plot1.plt
gnuplot plot2.plt
gnuplot plot3.plt
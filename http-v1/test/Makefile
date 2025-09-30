all: client server
client:tcp_cli.cc
	g++ -std=c++11 $^ -o $@
server:tcp_srv.cc
	g++ -std=c++11 $^ -o $@

.PHONY: clean
clean:
	rm -f client server
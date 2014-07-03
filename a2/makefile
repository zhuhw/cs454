all:
	g++ stringServer.cc -o stringServer
	g++ stringClient.cc -o stringClient -Wno-int-to-pointer-cast -lpthread

clean:
	rm stringServer stringClient

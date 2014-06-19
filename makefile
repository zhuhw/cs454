string:
	g++ stringServer.cc -o stringServer
	g++ stringClient.cc -o stringClient

clean:
	rm server client

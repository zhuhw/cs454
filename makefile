string:
	g++ stringServer.cc -o stringServer
	g++ stringClient.cc -o stringClient -Wno-int-to-void-pointer-cast

clean:
	rm server client

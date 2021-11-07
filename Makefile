CPPFLAGS=-std=c++2a -pg -Og -g
LINKFLAGS=-pg -Og -g -std=c++2a
LINKLIBS=-ldl -ltcc -lpthread -lstdc++fs

runtest: swarm
	./$^

main.o: main.cpp
	g++ -c $(CPPFLAGS) $^ -o $@

pathcache.o: pathcache.cpp
	g++ -c $(CPPFLAGS) $^ -o $@

swarm: main.o pathcache.o
	g++ $(LINKFLAGS) $^ -o $@ $(LINKLIBS)

clean:
	rm -f *.o swarm

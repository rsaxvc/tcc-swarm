runtest: swarm
	./$^

main.o: main.cpp
	g++ -c -g -pg -Og $^ -o $@

pathcache.o: pathcache.cpp
	g++ -c -g -pg -Og $^ -o $@

swarm: main.o
	g++ -g -pg -Og $^ -o $@ -ldl -ltcc -lpthread

clean:
	rm -f *.o swarm

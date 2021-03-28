runtest: swarm
	./$^

swarm: main.cpp
	g++ -g -Og $^ -o $@ -ldl -ltcc -lpthread

clean:
	rm -f *.o swarm

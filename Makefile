test: swarm
	./$^

swarm: main.cpp
	g++ $^ -o $@ -ldl -ltcc

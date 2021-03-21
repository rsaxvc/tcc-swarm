test: swarm
	./$^

swarm: main.c
	gcc $^ -o $@ -ldl -ltcc

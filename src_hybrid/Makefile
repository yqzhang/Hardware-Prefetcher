CC = g++
CCFLAGS = -g

cacheSim: CPU.o cache.o main.o memQueue.o prefetcher.o
	${CC} ${CCFLAGS} CPU.o cache.o main.o memQueue.o prefetcher.o -o cacheSim

CPU.o: CPU.C CPU.h mem-sim.h
	${CC} ${CCFLAGS} -c CPU.C

cache.o: cache.C cache.h
	${CC} ${CCFLAGS} -c cache.C

main.o: main.C mem-sim.h CPU.h cache.h memQueue.h prefetcher.h
	${CC} ${CCFLAGS} -c main.C

memQueue.o: memQueue.C memQueue.h mem-sim.h cache.h
	${CC} ${CCFLAGS} -c memQueue.C

prefetcher.o: prefetcher.C prefetcher.h mem-sim.h
	${CC} ${CCFLAGS} -c prefetcher.C

clean:
	rm -f *.o cacheSim

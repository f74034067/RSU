CC = gcc

PROGS = Rsu
LIBS =  -lm -lpthread

all:	${PROGS}

Rsu: Rsu.c TrafficLightState.c
		${CC} -g Rsu.c TrafficLightState.c -o Rsu ${LIBS}

clean:
		rm -f ${PROGS} ${CLEANFILES}

CC=mpicc
CFLAGS=-O3 -lm -Wall
RES=-DPRINT_RESULTS
CONV=-DTEST_CONV

all: jacobi

jacobi: jacobi_mpi.c utils.c
	$(CC) $(CFLAGS) $(RES) $(CONV) jacobi_mpi.c utils.c -o jacobi


clean:
	rm jacobi

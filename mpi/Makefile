CC=mpicc
CFLAGS=-O3 -Wall
RES=-DPRINT_RESULTS
##CONV=-DTEST_CONV

all: jacobi gauss

jacobi: jacobi_mpi.c utils.c
	$(CC) $(CFLAGS) $(CONV) $(RES) jacobi_mpi.c utils.c -o jacobi -lm
gauss: gauss_mpi.c utils.c
	$(CC) $(CFLAGS) $(CONV) $(RES) gauss_mpi.c utils.c -o gauss -lm
clean:
	rm jacobi gauss

clean_res:
	rm *.out

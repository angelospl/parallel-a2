#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include "mpi.h"
#include "utils.h"
#define CONV 5

int main(int argc, char ** argv) {
    int rank,size;
    int global[2],local[2]; //global matrix dimensions and local matrix dimensions (2D-domain, 2D-subdomain)
    int global_padded[2];   //padded global matrix dimensions (if padding is not needed, global_padded=global)
    int grid[2],pad[2];            //processor grid dimensions
    int i,j,t;
    int global_converged=0,converged=0; //flags for convergence, global and per process
    MPI_Datatype dummy;     //dummy datatype used to align user-defined datatypes in memory
    double omega; 			//relaxation factor - useless for Jacobi

    struct timeval tts,ttf,tcs,tcf;   //Timers: total-> tts,ttf, computation -> tcs,tcf
    double ttotal=0,tcomp=0,total_time,comp_time;

    double ** U, ** u_current, ** u_previous, ** swap; //Global matrix, local current and previous matrices, pointer to swap between current and previous


    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD,&size);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    printf("%d\n",size);

    //----Read 2D-domain dimensions and process grid dimensions from stdin----//

    if (argc!=5) {
        fprintf(stderr,"Usage: mpirun .... ./exec X Y Px Py");
        exit(-1);
    }
    else {
        global[0]=atoi(argv[1]);
        global[1]=atoi(argv[2]);
        grid[0]=atoi(argv[3]);
        grid[1]=atoi(argv[4]);
    }

    //----Create 2D-cartesian communicator----//
	//----Usage of the cartesian communicator is optional----//

    MPI_Comm CART_COMM;         //CART_COMM: the new 2D-cartesian communicator
    int periods[2]={0,0};       //periods={0,0}: the 2D-grid is non-periodic
    int rank_grid[2];           //rank_grid: the position of each process on the new communicator

    MPI_Cart_create(MPI_COMM_WORLD,2,grid,periods,0,&CART_COMM);    //communicator creation
    MPI_Cart_coords(CART_COMM,rank,2,rank_grid);	                //rank mapping on the new communicator

    //----Compute local 2D-subdomain dimensions----//
    //----Test if the 2D-domain can be equally distributed to all processes----//
    //----If not, pad 2D-domain----//
    for (i=0;i<2;i++) {
        if (global[i]%grid[i]==0) {
            local[i]=global[i]/grid[i];
            global_padded[i]=global[i];
            pad[i]=0;
        }
        else {
            local[i]=(global[i]/grid[i])+1;
            global_padded[i]=local[i]*grid[i];
            pad[i]=global_padded[i]-global[i];
        }
    }

	//Initialization of omega
    omega=2.0/(1+sin(3.14/global[0]));

    //----Allocate global 2D-domain and initialize boundary values----//
    //----Rank 0 holds the global 2D-domain----//
    if (rank==0) {
        U=allocate2d(global_padded[0],global_padded[1]);
        init2d(U,global[0],global[1]);
        print2d(U,global[0],global[1]);
    }
    //----Allocate local 2D-subdomains u_current, u_previous----//
    //----Add a row/column on each size for ghost cells----//

    u_previous=allocate2d(local[0]+2,local[1]+2);
    u_current=allocate2d(local[0]+2,local[1]+2);
    //----Distribute global 2D-domain from rank 0 to all processes----//

 	//----Appropriate datatypes are defined here----//
	/*****The usage of datatypes is optional*****/

    //----Datatype definition for the 2D-subdomain on the global matrix----//

    MPI_Datatype global_block;
    MPI_Type_vector(local[0],local[1],global_padded[1],MPI_DOUBLE,&dummy);
    MPI_Type_create_resized(dummy,0,sizeof(double),&global_block);
    MPI_Type_commit(&global_block);

    //----Datatype definition for the 2D-subdomain on the local matrix----//

    MPI_Datatype local_block;
    MPI_Type_vector(local[0],local[1],local[1]+2,MPI_DOUBLE,&dummy);
    MPI_Type_create_resized(dummy,0,sizeof(double),&local_block);
    MPI_Type_commit(&local_block);

    //----Rank 0 defines positions and counts of local blocks (2D-subdomains) on global matrix----//
    int * scatteroffset, * scattercounts;
    if (rank==0) {
        scatteroffset=(int*)malloc(size*sizeof(int));
        scattercounts=(int*)malloc(size*sizeof(int));
        for (i=0;i<grid[0];i++)
            for (j=0;j<grid[1];j++) {
                scattercounts[i*grid[1]+j]=1;
                scatteroffset[i*grid[1]+j]=(local[0]*local[1]*grid[1]*i+local[1]*j);
            }
    }

    //----Rank 0 scatters the global matrix----//
    MPI_Scatterv (&U[0][0],scattercounts,scatteroffset,global_block,&u_previous[1][1],1,local_block,0,CART_COMM);
    //printf("Eimai o rank %d %d\n",rank_grid[0],rank_grid[1]);
    //print2d(u_previous,local[0]+2,local[1]+2);
    if (rank==0) free2d(U);
	//----Define datatypes or allocate buffers for message passing----//
	//*************TODO*******************//
	MPI_Datatype local_column;
  MPI_Type_vector(local[0],1,local[1]+2,MPI_DOUBLE,&dummy);
  MPI_Type_create_resized(dummy,0,sizeof(double),&local_column);
  MPI_Type_commit(&local_column);

  MPI_Datatype local_row;
  MPI_Type_contiguous(local[1],MPI_DOUBLE,&dummy);
  MPI_Type_create_resized(dummy,0,sizeof(double),&local_row);
  MPI_Type_commit(&local_row);
	//************************************//


    //----Find the 4 neighbors with which a process exchanges messages----//
    //---Define the iteration ranges per process-----//
	//*************TODO*******************//
    int top, bot,left,right;
    int i_min,i_max,j_min,j_max;
    int bot_i_min,bot_j_min,top_i_min,top_j_min,lt_i_min,lt_j_min,rt_i_min,rt_j_min;
    bot_i_min=bot_j_min=top_i_min=top_j_min=lt_i_min=lt_j_min=rt_i_min=rt_j_min=-1;
    top=-1;
    bot=-1;
    left=-1;
    right=-1;
    if (rank_grid[0]==0 && rank_grid[1]==0) {     //  process upper left corner
      right=1;
      bot=1;
      i_min=2;
      j_min=2;
      i_max=local[0];
      j_max=local[1];
      rt_i_min=1;
      rt_j_min=local[1];
      bot_i_min=local[0];
      bot_j_min=1;
    }
    else if (rank_grid[0]==0 && rank_grid[1]==(grid[1]-1)) {  //process upper right corner
      left=1;
      bot=1;
      i_min=2;
      j_min=1;
      i_max=local[0];
      j_max=local[1]-1-pad[1];
      lt_i_min=1;
      lt_j_min=1;
      bot_i_min=i_max;
      bot_j_min=1;
    }
    else if (rank_grid[0]==(grid[0]-1) && rank_grid[1]==0) {  //process bottom left corner
      top=1;
      right=1;
      i_min=1;
      j_min=2;
      i_max=local[0]-1-pad[0];
      j_max=local[1];
      top_i_min=1;
      top_j_min=1;
      rt_i_min=1;
      rt_j_min=local[1];
    }
    else if (rank_grid[0]==(grid[0]-1) && rank_grid[1]==(grid[1]-1)) {  //process bottom right corner
      top=1;
      left=1;
      i_min=1;
      j_min=1;
      i_max=local[0]-1-pad[0];
      j_max=local[1]-1-pad[1];
      top_i_min=1;
      top_j_min=1;
      lt_i_min=1;
      lt_j_min=1;
    }
    else if (rank_grid[1]==0) { //process first column
      right=1;
      top=1;
      bot=1;
      i_min=1;
      j_min=2;
      i_max=local[0];
      j_max=local[1];
      top_i_min=top_j_min=1;
      rt_i_min=1;
      rt_j_min=local[1];
      bot_i_min=local[0];
      bot_j_min=1;
    }
    else if (rank_grid[1]==(grid[1]-1)) { //process last column
      left=1;
      top=1;
      bot=1;
      i_min=1;
      j_min=1;
      i_max=local[0];
      j_max=local[1]-1-pad[1];
      top_i_min=top_j_min=lt_i_min=lt_j_min=1;
      bot_i_min=local[0];
      bot_j_min=1;
    }
    else if (rank_grid[0]==0) { //  process first row
      bot=1;
      left=1;
      right=1;
      i_min=2;
      j_min=1;
      i_max=local[0];
      j_max=local[1];
      bot_i_min=local[0];
      bot_j_min=1;
      rt_i_min=1;
      rt_j_min=local[1];
      lt_i_min=lt_j_min=1;
    }
    else if (rank_grid[0]==grid[0]-1) { // process last row
      top=1;
      left=1;
      right=1;
      i_min=1;
      j_min=1;
      i_max=local[0]-1-pad[0];
      j_max=local[1];
      top_i_min=top_j_min=lt_i_min=lt_j_min=1;
      rt_i_min=1;
      rt_j_min=local[1];
    }
    else {  // process in the middle
      top=1;
      bot=1;
      right=1;
      left=1;
      i_min=j_min=1;
      i_max=local[0];
      j_max=local[1];
      top_i_min=top_j_min=lt_i_min=lt_j_min=1;
      bot_i_min=local[0];
      bot_j_min=1;
      rt_i_min=1;
      rt_j_min=local[1];
    }
    //printf("Eimai o rank %d %d me i_min %d i_max %d j_min %d j_max %d\n",rank_grid[0],rank_grid[1],i_min,i_max,j_min,j_max);
	/*Make sure you handle non-existing
		neighbors appropriately*/
	//************************************//
	/*Three types of ranges:
		-internal processes
		-boundary processes
		-boundary processes and padded global array
	*/
	//************************************//
 	//----Computational core----//
  for (i = 0; i < local[0]+2; i++) {
    for (j = 0; j < local[1]+2; j++) {
      u_current[i][j]=u_previous[i][j];
    }
  }
  printf("Eimai o node %d %d me j_min %d me j_max %d\n",rank_grid[0],rank_grid[1],j_min,j_max);
  //MPI_Barrier(MPI_COMM_WORLD);
  gettimeofday(&tts, NULL);
  MPI_Status status;
  converged=0;
  for (t=0;t<256 && !global_converged;t++) {
	 	//*************TODO*******************//
    swap=u_previous;
    u_previous=u_current;
    u_current=swap;
    if (bot==1) {
      //printf("Eimai o rank %d %d kai kanw send bot. Stelnw ston %d. xekinaw apo %d %d\n",rank_grid[0],rank_grid[1],(rank_grid[0]+1)*grid[1]+rank_grid[1],bot_i_min,bot_j_min );
      MPI_Send(&u_previous[bot_i_min][bot_j_min],1,local_row,(rank_grid[0]+1)*grid[1]+rank_grid[1],20,MPI_COMM_WORLD);
      MPI_Recv(&u_previous[bot_i_min+1][bot_j_min],1,local_row,(rank_grid[0]+1)*grid[1]+rank_grid[1],10,MPI_COMM_WORLD,&status);
    }
    if (right==1) {
      // printf("Eimai o rank %d %d kai kanw send right. Stelnw ston %d. xekinaw apo %d %d\n",rank_grid[0],rank_grid[1],(rank_grid[0])*grid[1]+rank_grid[1]+1,rt_i_min,rt_j_min );
      MPI_Send(&u_previous[rt_i_min][rt_j_min],1,local_column,rank_grid[0]*grid[1]+rank_grid[1]+1,30,MPI_COMM_WORLD);
      MPI_Recv(&u_previous[rt_i_min][rt_j_min+1],1,local_column,(rank_grid[0])*grid[1]+rank_grid[1]+1,40,MPI_COMM_WORLD,&status);
    }
    if (top!=-1) {
      // printf("Eimai o rank %d %d kai kanw send top. Stelnw ston %d. xekinaw apo %d %d\n",rank_grid[0],rank_grid[1],(rank_grid[0]-1)*grid[1]+rank_grid[1],top_i_min,top_j_min );
      MPI_Send(&u_previous[top_i_min][top_j_min],1,local_row,(rank_grid[0]-1)*grid[1]+rank_grid[1],10,MPI_COMM_WORLD);
      MPI_Recv(&u_previous[top_i_min-1][top_j_min],1,local_row,(rank_grid[0]-top)*grid[1]+rank_grid[1],20,MPI_COMM_WORLD,&status);
    }
    if (left!=-1) {
      //printf("Eimai o rank %d %d kai kanw send left\n",rank_grid[0],rank_grid[1] );
      MPI_Send(&u_previous[lt_i_min][lt_j_min],1,local_column,rank_grid[0]*grid[1]+rank_grid[1]-1,40,MPI_COMM_WORLD);
      MPI_Recv(&u_previous[lt_i_min][lt_j_min-1],1,local_column,(rank_grid[0])*grid[1]+rank_grid[1]-1,30,MPI_COMM_WORLD,&status);
    }
    //MPI_Barrier(MPI_COMM_WORLD);
    //print2d(u_previous,local[0]+2,local[1]+2);
		/*Compute and Communicate*/
    for (j=j_min;j<=j_max;j++) {
      for (i=i_min;i<=i_max;i++) {
          u_current[i][j]=(u_previous[i-1][j]+u_previous[i+1][j]+u_previous[i][j-1]+u_previous[i][j+1])/4.0;
      }
    }
    MPI_Barrier(MPI_COMM_WORLD);
		/*Add appropriate timers for computation*/
    #ifdef TEST_CONV
        if (t%CONV==0) {
          converged=converge(u_previous,u_current,local[0],local[1]); //converge from 1 1 to local[0] local[1]
          MPI_Allreduce(&converged,&global_converged,1,MPI_INT,MPI_PROD,MPI_COMM_WORLD);
		}
		#endif
		//************************************//
  } //for time
  MPI_Barrier(MPI_COMM_WORLD);
  // for (i = 0; i < local[0]+2; i++) {
  //   for (j = 0; j<local[1]+2; j++) {
  //     printf("%lf",u_current[i][j]);
  //   }
  //   printf("\n");
  // }
    // gettimeofday(&ttf,NULL);
    //
    // ttotal=(ttf.tv_sec-tts.tv_sec)+(ttf.tv_usec-tts.tv_usec)*0.000001;
    //
    // MPI_Reduce(&ttotal,&total_time,1,MPI_DOUBLE,MPI_MAX,0,MPI_COMM_WORLD);
    // MPI_Reduce(&tcomp,&comp_time,1,MPI_DOUBLE,MPI_MAX,0,MPI_COMM_WORLD);



    //----Rank 0 gathers local matrices back to the global matrix----//

    // if (rank==0) {
    //         U=allocate2d(global_padded[0],global_padded[1]);
    // }


	//*************TODO*******************//
  //MPI_Gatherv(&u_current[1][1],1,local_block,&U[0][0],scattercounts,scatteroffset,global_block,0,CART_COMM);

     //************************************//





	//----Printing results----//

	//**************TODO: Change "Jacobi" to "GaussSeidelSOR" or "RedBlackSOR" for appropriate printing****************//
    // if (rank==0) {
    //     printf("Jacobi X %d Y %d Px %d Py %d Iter %d ComputationTime %lf TotalTime %lf midpoint %lf\n",global[0],global[1],grid[0],grid[1],t,comp_time,total_time,U[global[0]/2][global[1]/2]);
    //
    //     #ifdef PRINT_RESULTS
    //     char * s=malloc(50*sizeof(char));
    //     sprintf(s,"resJacobiMPI_%dx%d_%dx%d",global[0],global[1],grid[0],grid[1]);
    //     fprint2d(s,U,global[0],global[1]);
    //     free(s);
    //     #endif
    //
    // }
    MPI_Finalize();
    return 0;
}

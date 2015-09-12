# include <stdlib.h>
# include <math.h>
# include <time.h>
# include <stdio.h>
# include "mpi.h"
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

#define MAX_LIGAND_NUMBER 20000
#define MAX_LIGAND_NAME_LENGTH 25
#define NODE_FAILURE_TIME 150.0

	int failure_any_node=0;
	int givemework[1],anyworkreq[1],workdone[1],ligand_to_process,just_sleep[1],just_exit[1];
	int server_response[1];
	int total_process, rank,namelen,ligand_processed,ligand_processed_and_processing,total_ligand=0,start,sleeping;
	
	char buffer[MAX_LIGAND_NUMBER][MAX_LIGAND_NAME_LENGTH];
	char processor_name[MPI_MAX_PROCESSOR_NAME];
	int legand_no;

void master();
void slave();

int main ( int argc, char *argv[] )
{
	MPI_Init (&argc, &argv );
	MPI_Get_processor_name(processor_name, &namelen);
	MPI_Comm_size(MPI_COMM_WORLD,&total_process);
	MPI_Comm_rank(MPI_COMM_WORLD,&rank);
	
	int ligand_number;
	FILE *infile;	
	
	if(rank==0){
		
				// Generate the Array of Ligand List from Ligand Folder
				char *inname = "ligandlist";
				char ligand_name[MAX_LIGAND_NAME_LENGTH]; 
				infile = fopen(inname, "r");
		        if (!infile){
							printf("Couldn't open file %s for reading.\n", inname);
							return 0;
							}
				printf("Opened Ligand file %s for reading.\n", inname); // WHAT is the PURPOSE
				ligand_number=0;
				while (fgets(ligand_name, sizeof(ligand_name), infile)) {
							char *nlptr = strchr(ligand_name, '\n');
							if (nlptr) *nlptr = '\0';
							strcpy(buffer[ligand_number],ligand_name);
							ligand_number++;
							}//end of while
				total_ligand=ligand_number;
				//Array Generation Complete.
	}
	
	MPI_Bcast(&buffer, MAX_LIGAND_NUMBER*MAX_LIGAND_NAME_LENGTH, MPI_CHAR, 0, MPI_COMM_WORLD);
	
	//Master Processor
	if (rank==0){
		master();
		//printf("\n\nMaster is done here \n\n");
		
	} ///////////End Server Node///////////
		
	//Slave Processor
	else{
		slave();
		//printf("Node processor: %d is done here\n ",rank);
	}//end of if
	printf("Node= %s with Rank= %d is terminated\n",processor_name,rank);
	
	if(failure_any_node==1 && rank==0 ){
			sleep(25);
			printf("------------------------------------------------------------------------------------\n");
			printf("All ligand has been processed successfully.But MPI program may not \n terminate normally due the failoure  of one or more node.\n Please press Ctrl+C to exit\n");
			printf("------------------------------------------------------------------------------------\n");
		}
		
  MPI_Finalize ( );
  return 0;
}


void master(){
	MPI_Status status2;
	ligand_processed=0;
	ligand_processed_and_processing=0;
	ligand_to_process=0;
	int i=1,j=1,flag,temp,last_assigned_ligand[total_process],process_status[total_process];
	double wtime, last_assigned_ligand_time[total_process];
	
	for(j=1; j<total_process; j++){
		process_status[j]=1;
		last_assigned_ligand[j]=-1;
		last_assigned_ligand_time[j]=0.0;
		}
		
	while(i!=total_process){
		MPI_Recv(anyworkreq,1,MPI_INT,MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&status2);
		
		if(status2.MPI_TAG==11){
			if (ligand_processed_and_processing < total_ligand){															
				MPI_Send(&ligand_to_process,1,MPI_INT,status2.MPI_SOURCE,11,MPI_COMM_WORLD);//GiveWOrk.
				
				last_assigned_ligand[status2.MPI_SOURCE]=ligand_to_process;
				last_assigned_ligand_time[status2.MPI_SOURCE]=MPI_Wtime ( );
				
				ligand_to_process=ligand_to_process+1;
				ligand_processed_and_processing=ligand_processed_and_processing+1;
				}//end of if
			else if((ligand_processed < total_ligand) && (ligand_processed_and_processing==total_ligand)){
				
				flag=1;
				for(j=1; j<total_process; j++){																		
					if(last_assigned_ligand[j] != -1 ){
						wtime=fabs(MPI_Wtime ( ) - last_assigned_ligand_time[j]);
						if( wtime > NODE_FAILURE_TIME ){//printf("Yes\n");
							temp=last_assigned_ligand[j];
							MPI_Send(&temp,1,MPI_INT,status2.MPI_SOURCE,11,MPI_COMM_WORLD);//GiveWOrk.
							last_assigned_ligand[status2.MPI_SOURCE]=temp;
							last_assigned_ligand_time[status2.MPI_SOURCE]=MPI_Wtime ( );
							last_assigned_ligand[j]=-1;
							last_assigned_ligand_time[j]=0;
							process_status[j]=0;
							failure_any_node=1;
							flag=0;  break;
						}
					}																				
				}
				if(flag)	
					MPI_Send(NULL,0,MPI_INT,status2.MPI_SOURCE,12,MPI_COMM_WORLD); // Go to Sleep
				}//end of else if
			else if (ligand_processed == total_ligand ){//all work done																	
				
				i++;
				MPI_Send(NULL,0,MPI_INT,status2.MPI_SOURCE,13,MPI_COMM_WORLD);																																
				//printf("\n Procssing is done, closing all processes\n");
				
			/*	for (j=1;j<total_process;j++){
					if( process_status[j]==1 )
						MPI_Send(NULL,0,MPI_INT,j,13,MPI_COMM_WORLD);
				}
				break;		*/	
					
			}//end of if
		}
		else if(status2.MPI_TAG==22){
			ligand_processed=ligand_processed+1;
			
			last_assigned_ligand[status2.MPI_SOURCE]=-1;
			last_assigned_ligand_time[status2.MPI_SOURCE]=0;											
			
			if (ligand_processed_and_processing < total_ligand){
				MPI_Send(&ligand_to_process,1,MPI_INT,status2.MPI_SOURCE,11,MPI_COMM_WORLD);//GiveWOrk.
				
				last_assigned_ligand[status2.MPI_SOURCE]=ligand_to_process;
				last_assigned_ligand_time[status2.MPI_SOURCE]=MPI_Wtime ( );
				
				ligand_to_process=ligand_to_process+1;
				ligand_processed_and_processing=ligand_processed_and_processing+1;
				}//end of if
			else if((ligand_processed < total_ligand) && (ligand_processed_and_processing==total_ligand)){
				
				flag=1;
				for(j=1; j<total_process; j++){																		
					if(last_assigned_ligand[j] != -1 ){
						wtime=fabs(MPI_Wtime ( ) - last_assigned_ligand_time[j]);
						if( wtime > NODE_FAILURE_TIME){
							temp=last_assigned_ligand[j];
							MPI_Send(&temp,1,MPI_INT,status2.MPI_SOURCE,11,MPI_COMM_WORLD);//GiveWOrk.
							last_assigned_ligand[status2.MPI_SOURCE]=temp;
							last_assigned_ligand_time[status2.MPI_SOURCE]=MPI_Wtime ( );
							last_assigned_ligand[j]=-1;
							last_assigned_ligand_time[j]=0;
							process_status[j]=0;
							failure_any_node=1;
							flag=0;  break;
						}
					}																			
				}
				if(flag)					
					MPI_Send(NULL,0,MPI_INT,status2.MPI_SOURCE,12,MPI_COMM_WORLD); // Go to Sleep
				}//end of else if
			else if ( ligand_processed == total_ligand ){//all work done
				
				i++;	
				MPI_Send(NULL,0,MPI_INT,status2.MPI_SOURCE,13,MPI_COMM_WORLD);																															
				//printf("\n Procssing is done, programme is closing on nodes(within tag=22) \n");
				
		/*		for (j=1;j<total_process;j++){
					if( process_status[j]==1 ) 
						MPI_Send(NULL,0,MPI_INT,j,13,MPI_COMM_WORLD);	
				}			
				break;		*/
				
			}//end of else if
		}//end of else if 

	}//end of while
}//end of master
									
			
void slave(){
		MPI_Status status;
		printf("Started node: %s with rank %d\n",processor_name,rank);
		start=1;sleeping=0;
		while (1){
				givemework[0]=0;
				if (start==1 || sleeping==1){ // wheather the loop is running the programme for first time or it was sleeping.											
											MPI_Send(NULL,0,MPI_INT,0,11,MPI_COMM_WORLD);
											}				
				MPI_Recv(server_response,1,MPI_INT,0,MPI_ANY_TAG,MPI_COMM_WORLD,&status);				
				start=0; // changing the start value to no .
				if (status.MPI_TAG==11){ //server gave node a work.
										legand_no=server_response[0];
										system("cp -r Vina /dev/shm/");
										char cmd[500]="/dev/shm/Vina/vina --config /dev/shm/Vina/conf.txt --ligand ./Ligand/";
										strcat(cmd,buffer[legand_no]);
										strcat(cmd," --out /dev/shm/");
										strcat(cmd,buffer[legand_no]);
										strcat(cmd,".pdbqt --log /dev/shm/");
										strcat(cmd,buffer[legand_no]);
										strcat(cmd,".txt>/dev/null");
										system(cmd);
										system("mv /dev/shm/*.pdbqt* Output/");
										cmd[0]='\0';
										strcat(cmd,"mv  Ligand/");
										strcat(cmd,buffer[legand_no]);
										strcat(cmd," ProcessedLigand/");
										system(cmd);
										workdone[0]=legand_no;
										printf("Ligand No=%d  Ligand Name=%s is processed by %s with rank=%d\n",legand_no,buffer[legand_no],processor_name,rank);
										MPI_Send(workdone,1,MPI_INT,0,22,MPI_COMM_WORLD);
										}// end of if
				else if (status.MPI_TAG==12){ //server commanded node to sleep.
										//printf("\n Got sleeping command (tag=%d) on: %d\n",status.MPI_TAG,rank);
										system("sleep 2"); //sleep for 30 seconds
										//printf("sleeping is done on : %d \n", rank);
										sleeping=1;  // sleeping =yes
										}//end of else if
				else if (status.MPI_TAG==13){ //server commanded node to sleep
										//printf(" got exit command (tag=%d) on: %d\n",status.MPI_TAG,rank);										
										break;
										}//end of else if
				
				}//end of while
			
		} //end of slave

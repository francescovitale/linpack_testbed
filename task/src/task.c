#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <inttypes.h>
#include <math.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include "lib/rt-lib.h"
#include <arpa/inet.h>
#include <netinet/in.h>

#define MAXCHAR 512
#define PERIOD 1000000 //Period in microseconds

//Static Variables
static int n_reps;
static int arr_size;
static int N;
static char* filepath;
static char* ip;
static int port;


long long timeInMicro(void) {
  struct timespec spec;
  clock_gettime(CLOCK_REALTIME, &spec);
  return (((long long)spec.tv_sec)*1000000)+(spec.tv_nsec/1000);

}

long long timeInNano(void) {
  struct timespec spec;
  clock_gettime(CLOCK_REALTIME, &spec);
  return (((long long)spec.tv_sec)*1000000000)+(spec.tv_nsec);

}

char* build_invocation_string(){
	int n_reps_string_size = sizeof(char)*(int)log10(n_reps)+1;
	int arr_size_string_size = sizeof(char)*(int)log10(arr_size)+1;
    int linpack_filepath_size = strlen(filepath)+1;

	char n_reps_string[n_reps_string_size+1];
	char arr_size_string[arr_size_string_size+1];

	sprintf(n_reps_string, "%d", n_reps);
	sprintf(arr_size_string, "%d", arr_size);

	char* invocation_string = (char*)malloc(sizeof(char)*(linpack_filepath_size+arr_size_string_size+n_reps_string_size+1));

	invocation_string = strcpy(invocation_string, "");
	invocation_string = strcat(invocation_string, filepath);
	invocation_string = strcat(invocation_string, " ");
	invocation_string = strcat(invocation_string, n_reps_string);
	invocation_string = strcat(invocation_string, " ");
	invocation_string = strcat(invocation_string, arr_size_string);
	return invocation_string;
}


int create_socket(char* ip, int port)
{
    int sockfd,error;
    struct sockaddr_in serv_addr;
    const int enable = 1;

    sockfd=socket(AF_INET,SOCK_STREAM,0);
    if(sockfd == -1){
        printf("Can't create socket\n");
        return -1;
    }   

    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))<0){
        printf("setsockopt(SO_REUSEADDR) failed");
    }
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int))<0){
        printf("setsockopt(SO_REUSEPORT) failed");
    }

    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=INADDR_ANY;
    serv_addr.sin_port=htons(port);
    serv_addr.sin_addr.s_addr = inet_addr(ip);

    error=fcntl(sockfd,F_SETFL,0);
    //printf("error value after fnctl: %d\n",error);
    error=bind(sockfd,(struct sockaddr*) &serv_addr,sizeof(serv_addr));
    //printf("error value after bind: %d\n",error);
    error=listen(sockfd,7);
    //printf("error value after listen: %d\n",error);
    if(error == -1){
        printf("Error: create_socket fail.\n");
    return -1;
    }

    return sockfd;
}

void close_socket(int sock)
{
    close(sock);
    return;
}

int check_msg(int sockfd, char buffer[512])
{
	int exit_cond=0;
	int dim=0;


	if ((dim=read(sockfd,buffer,512))<0)
	{
		printf("could not read acknowledgements\n");
		close_socket(sockfd);
        exit_cond=1;
	}
	else
	{	
		//printf("ack received");
		exit_cond=0;
	}
	return exit_cond;
}

void* task_loop(void* par){
	
	long long start;
	long long finish;
	long long diff[N];
    char* program_invocation_string;
    char buffer[512];
    int server_sockfd;
    int client_sockfd;
    int exit_cond;
    struct sockaddr_in client_addr;
    socklen_t clilen;

	program_invocation_string = build_invocation_string();

	//printf("%s\n", program_invocation_string);

    server_sockfd = create_socket(ip,port);
    //printf("server_sockfd after creating the socket: %d\n",server_sockfd);
    if(server_sockfd==-1){
		printf("error: socket could not be created\n");
		pthread_exit(NULL);
	}
    clilen = sizeof(client_addr);
	client_sockfd = accept(server_sockfd,(struct sockaddr*)&client_addr,&clilen);
    //printf("client_sockfd after accepting the connection: %d\n",server_sockfd);
    exit_cond = check_msg(client_sockfd, buffer);
    if(exit_cond == 1){
        pthread_exit(NULL);
    }
	
	//Utility
	int i=0;
	
    periodic_thread *th = (periodic_thread *) par;
    start_periodic_timer(th,0);

	while(i < N)
	{
		wait_next_activation(th);
		start=timeInNano();
		//printf("execution n. %d\n", i);
        int status = system(program_invocation_string);
		finish = timeInNano();
		diff[i] = (finish - start);
        printf("run %d:%lld\n",i,diff[i]);
		i++;
	}
	wait_next_activation(th);
    
    free(program_invocation_string);
    close_socket(server_sockfd);
    close_socket(client_sockfd);
	pthread_exit(NULL);
}

int main(int argc,char* argv[])
{
  	if (argc != 7){
        printf("Insert the array size, the number of repetitions, the number of the period task instances, the absolute path where the linpack executable is, the ip of the network interface to bind the socket at, and the port to listen messages from\n");
        return 0;
    }
    else
    {
	    arr_size=atoi(argv[1]);
		n_reps=atoi(argv[2]);
        N=atoi(argv[3]);
        filepath = malloc(sizeof(char)*(strlen(argv[4])+1));
        strcpy(filepath,argv[4]);
        ip = malloc(sizeof(char)*(strlen(argv[5])+1));
        strcpy(ip,argv[5]);
        port = atoi(argv[6]);
	}

	// Init thread attr.
	pthread_t controller_thread;
	pthread_attr_t myattr;
	struct sched_param myparam;

	pthread_attr_init(&myattr);
	pthread_attr_setschedpolicy(&myattr, SCHED_FIFO);
	pthread_attr_setinheritsched(&myattr, PTHREAD_EXPLICIT_SCHED);

	// REPLICA THREAD
	periodic_thread controller_th;
	controller_th.period = PERIOD;
	controller_th.priority = 50;

	myparam.sched_priority = controller_th.priority;
	pthread_attr_setschedparam(&myattr, &myparam);
	pthread_create(&controller_thread,&myattr,task_loop,(void*)&controller_th);

	pthread_attr_destroy(&myattr);

	//Wait controller_thread to finish
	pthread_join(controller_thread,NULL);

    free(filepath);
	return 0;
}

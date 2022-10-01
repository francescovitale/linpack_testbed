#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
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

#define MAXCHAR 512
#define PERIOD 1000000 //Period in microseconds

//Static Variables
static int n_reps;
static int arr_size;
static int N;


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

	char n_reps_string[n_reps_string_size+1];
	char arr_size_string[arr_size_string_size+1];

	sprintf(n_reps_string, "%d", n_reps);
	sprintf(arr_size_string, "%d", arr_size);

	char* invocation_string = (char*)malloc(sizeof(char)*(11+arr_size_string_size+n_reps_string_size+1));


	invocation_string = strcpy(invocation_string, "");
	invocation_string = strcat(invocation_string, "./linpack");
	invocation_string = strcat(invocation_string, " ");
	invocation_string = strcat(invocation_string, n_reps_string);
	invocation_string = strcat(invocation_string, " ");
	invocation_string = strcat(invocation_string, arr_size_string);

	return invocation_string;

}

void* task_loop(void* par){
	
	long long start;
	long long finish;
	long long diff[N];
	char* program_invocation_string;

	program_invocation_string = build_invocation_string();

	//printf("%s\n", program_invocation_string);

	
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
	pthread_exit(NULL);
}

int main(int argc,char* argv[])
{
  	if (argc != 4){
        printf("Insert the array size, the number of repetitions, and the number of the period task instances\n");
        return 0;
    }
    else
    {
	    arr_size=atoi(argv[1]);
		n_reps=atoi(argv[2]);
        N=atoi(argv[3]);
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

	return 0;
}

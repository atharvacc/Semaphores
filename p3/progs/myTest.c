#include <assert.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tps.h>
#include <sem.h>


static void *thread1(void  *arg){

tps_init(5);
tps_create();
tps_destroy();
return NULL;
}

int main(int argc, char **argv)
{
	pthread_t tid;

	pthread_create(&tid, NULL, thread1, NULL);
	pthread_join(tid, NULL);
	printf("tid was assigned to %lu \n", tid);
	printf("Back in main \n");
}

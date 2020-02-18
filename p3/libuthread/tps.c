#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "queue.h"
#include "thread.h"
#include "tps.h"

struct page *page_t;

struct memoryStorage{
	pthread_t* tid;
	char* mmapPtr;
};

queue_t memoryQUEUE;


/* Find TID in queue */
static int find_tid(void *data, void *arg)
{
    struct memoryStorage  *a = (struct memoryStorage*)data;
    pthread_t match = (pthread_t)arg;
    if (a->tid == match){
        return 1;
    }
    return 0;
}


int tps_init(int segv)
{
	//TODO//
	printf("In tps init \n");
	memoryQUEUE = queue_create();
	printf("Done w tps init \n");
	return 0;
}

int tps_create(void)
{
	printf("In Create \n");
	pthread_t *curTid = malloc(sizeof(pthread_t));
	curTid = pthread_self();
	printf("CurTID is %d \n", curTid);
	void *mptr = NULL;
	mptr = mmap(NULL,TPS_SIZE, PROT_NONE, MAP_PRIVATE| MAP_ANON,-1,0); // Create memory
	if(mptr == MAP_FAILED){
		return -1;
	} // If failed in memory creation
	
	struct memoryStorage *tempStorage = malloc(sizeof(struct memoryStorage));
	tempStorage->tid = curTid;
	tempStorage->mmapPtr = (char*) mptr;
	
	queue_enqueue(memoryQUEUE, tempStorage);
	printf("Queue lenght in create is %d \n", queue_length(memoryQUEUE));
	printf("Done w create \n");
	return 0;
}

int tps_destroy(void)
{	printf("In Destory \n");
	pthread_t *curTid = malloc(sizeof(pthread_t));
	curTid = pthread_self();
	printf("current TID is %d \n", curTid);
	void* tempStorage = NULL;
	if(queue_iterate(memoryQUEUE,find_tid, (void*)curTid,(void**) &tempStorage) == 0){	
		if (tempStorage == NULL){
			printf("TID WAS NOT FOUND\n");
			return -1;
		} // If no tid is found
		else{
			printf("TID WAS FOUND\n");
			struct memoryStorage *temp = (struct memoryStorage*) tempStorage;
			munmap(temp->mmapPtr, TPS_SIZE);
			//free(temp-> mmapPtr);
			queue_delete(memoryQUEUE, temp);
		} // if tid is found then delete it 
	}
	printf("Done w Destroy \n");
	return 0;
}

int tps_read(size_t offset, size_t length, void *buffer)
{
	return 0;
}

int tps_write(size_t offset, size_t length, void *buffer)
{
	return 0;
}

int tps_clone(pthread_t tid)
{
	
	return 0;
}


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

struct page {
	char* mmapPtr;
	int refCounter;
};

struct memoryStorage{
	pthread_t* tid;
	struct page* myPage;
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

static int find_char(void *data, void *arg)
{
    struct memoryStorage  *a = (struct memoryStorage*)data;
    char* match = (char*)arg;
    if (a->myPage->mmapPtr == match){
        return 1;
    }
    return 0;
}

static void segv_handler(int sig, siginfo_t *si, __attribute__((unused)) void *context)
{
    /*
     * Get the address corresponding to the beginning of the page where the
     * fault occurred
     */
    void *p_fault = (void*)((uintptr_t)si->si_addr & ~(TPS_SIZE - 1));
	void* tempStorage = NULL;
    /*
     * Iterate through all the TPS areas and find if p_fault matches one of them
     */
	queue_iterate(memoryQUEUE,find_char, (void*)p_fault,(void**) &tempStorage);
	
    if (tempStorage != NULL)
        /* Printf the following error message */
        fprintf(stderr, "TPS protection error!\n");

    /* In any case, restore the default signal handlers */
    signal(SIGSEGV, SIG_DFL);
    signal(SIGBUS, SIG_DFL);
    /* And transmit the signal again in order to cause the program to crash */
    raise(sig);
}
int tps_init(int segv)
{
    if (segv) {
        struct sigaction sa;

        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_SIGINFO;
        sa.sa_sigaction = segv_handler;
        sigaction(SIGBUS, &sa, NULL);
        sigaction(SIGSEGV, &sa, NULL);
    }

	memoryQUEUE = queue_create();
	
	return 0;
}

int tps_create(void)
{
	pthread_t *curTid = malloc(sizeof(pthread_t));
	curTid = pthread_self();
	void *mptr = NULL;
	mptr = mmap(NULL,TPS_SIZE, PROT_NONE, MAP_PRIVATE| MAP_ANON,-1,0); // Create memory
	if(mptr == MAP_FAILED){
		return -1;
	} // If failed in memory creation
	
	struct memoryStorage *tempStorage = malloc(sizeof(struct memoryStorage));
	tempStorage->myPage = malloc(sizeof(struct page));
	tempStorage->tid = curTid;
	tempStorage->myPage->mmapPtr = (char*) mptr;
	
	queue_enqueue(memoryQUEUE, tempStorage);
	return 0;
}

int tps_destroy(void)
{	
	pthread_t *curTid = malloc(sizeof(pthread_t));
	curTid = pthread_self();
	void* tempStorage = NULL;
	if(queue_iterate(memoryQUEUE,find_tid, (void*)curTid,(void**) &tempStorage) == 0){	
		if (tempStorage == NULL){
			return -1;
		} // If no tid is found
		else{
			struct memoryStorage *temp = (struct memoryStorage*) tempStorage;
			munmap(temp->myPage->mmapPtr, TPS_SIZE);
			queue_delete(memoryQUEUE, temp);
		} // if tid is found then delete it 
	}
	return 0;
}

int tps_read(size_t offset, size_t length, void *buffer)
{
	if( ((offset+length) > TPS_SIZE) || (buffer == NULL) ){
		return -1;
	} // OUT OF BOUND
	pthread_t *curTid = malloc(sizeof(pthread_t));
	curTid = pthread_self();
	void* tempStorage = NULL;
	if(queue_iterate(memoryQUEUE,find_tid, (void*)curTid,(void**) &tempStorage) == 0){	
		if (tempStorage == NULL){
			return -1;
		} // If no tid is found
		else{
			struct memoryStorage *temp = (struct memoryStorage*) tempStorage;
			mprotect(temp->myPage->mmapPtr, TPS_SIZE, PROT_READ);
			memcpy(buffer, temp->myPage->mmapPtr + offset, length );
			mprotect(temp->myPage->mmapPtr, TPS_SIZE, PROT_NONE);
		}// TID WAS FOUND
	}
	return 0;
}

int tps_write(size_t offset, size_t length, void *buffer)
{
	if( ((offset+length) > TPS_SIZE) || (buffer == NULL) ){
		return -1;
	} // OUT OF BOUND
	pthread_t *curTid = malloc(sizeof(pthread_t));
	curTid = pthread_self();
	void* tempStorage = NULL;
	if(queue_iterate(memoryQUEUE,find_tid, (void*)curTid,(void**) &tempStorage) == 0){	
		if (tempStorage == NULL){
			return -1;
		} // If no tid is found
		else{
			struct memoryStorage *temp = (struct memoryStorage*) tempStorage;
			mprotect(temp->myPage->mmapPtr, TPS_SIZE, PROT_WRITE);
			memcpy( temp->myPage->mmapPtr + offset, buffer, length );
			mprotect(temp->myPage->mmapPtr, TPS_SIZE, PROT_NONE);
		}// TID WAS FOUND
	}
	return 0;
}

int tps_clone(pthread_t tid)
{
	pthread_t *curTid = malloc(sizeof(pthread_t));
	curTid = pthread_self();
	void* tempStorage = NULL;
	void* tempStorage1 = NULL;
	//Check if currently running thread has a TPS
	queue_iterate(memoryQUEUE,find_tid, (void*)curTid,(void**) &tempStorage);
	
	// Check if TPS is available for tid provided
	queue_iterate(memoryQUEUE,find_tid, (void*)tid,(void**) &tempStorage1);
	if(tempStorage != NULL){
		return -1;
	} // If Currently running thread did have a TPS
	else if (tempStorage1 == NULL){
		return -1;
	} // If the tid provided did not have a TPS
	else{
		
	void *mptr = NULL;
	mptr = mmap(NULL,TPS_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE| MAP_ANON,-1,0); // Create memory
	
	if(mptr == MAP_FAILED){
		return -1;
	} // If failed in memory creation

	struct memoryStorage *cloneStorage = malloc(sizeof(struct memoryStorage));
	cloneStorage->myPage = malloc(sizeof(struct page));
	cloneStorage->tid = curTid;
	cloneStorage->myPage->mmapPtr = (char*) mptr;
	struct memoryStorage *temp = (struct memoryStorage*) tempStorage1;
	mprotect(temp->myPage->mmapPtr, TPS_SIZE, PROT_READ);
	memcpy( cloneStorage->myPage->mmapPtr, temp->myPage->mmapPtr, TPS_SIZE);
	mprotect(cloneStorage->myPage->mmapPtr, TPS_SIZE, PROT_NONE);
	mprotect(temp->myPage->mmapPtr, TPS_SIZE, PROT_NONE);
	queue_enqueue(memoryQUEUE, cloneStorage);
	} // Can be cloned

	return 0;
}


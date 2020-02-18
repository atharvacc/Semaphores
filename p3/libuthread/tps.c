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

static queue_t memoryQUEUE;

typedef struct tps *tps_t;
typedef struct page *page_t;

/* memoryStorage stores the tid of the thread and a pointer to its page struct*/
struct memoryStorage{
	pthread_t* tid;
	char* mmapPtr;
};

// page structure containing the memory page's address and reference counter
struct page {
	void *tpsLocation;
	int reference_counter;
}

/* TODO: Phase 2 */

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

// segv_handler - segmentation fault handler
// distinguishes between real segmentation faults and TPS protection faults
static void segv_handler(int sig, siginfo_t *si, void *context)
{
    /*
     * Get the address corresponding to the beginning of the page where the
     * fault occurred
     */
    void *p_fault = (void*)((uintptr_t)si->si_addr & ~(TPS_SIZE - 1));

		page_t foundPage = NULL;

		queue_iterate(memoryQUEUE, find_tid, (void*)p_fault, (void**)&foundPage);

    /*
     * Iterate through all the TPS areas and find if p_fault matches one of them
     */
    if (foundPage != NULL)
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
	//TODO//
	if (segv) {
  	struct sigaction sa;

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = segv_handler;
    sigaction(SIGBUS, &sa, NULL);
    sigaction(SIGSEGV, &sa, NULL);
  }

	memoryQUEUE = queue_create();
	printf("Done creating memoryQUEUE \n ");
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
{
	pthread_t *curTid = malloc(sizeof(pthread_t));
	curTid = pthread_self;
	void* tempStorage = NULL;
	if(queue_iterate(memoryQUEUE, find_tid, (void*)curTid, (void**) &tempStorage) == 0) {
		printf("In Destory\n");
		printf("Len of queue is :%d \n", queue_length(memoryQUEUE));
		if (tempStorage == NULL){
			return -1;
		} // If no tid is found
		else{
			printf("In here\n");
			struct memoryStorage *temp = (struct memoryStorage*) tempStorage;
			free(temp-> mmapPtr);
			queue_delete(memoryQUEUE, temp);
		} // if tid is found then delete it
	}
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
	// need to allocate a new TPS object for the calling thread, but have the page structure be shared with the target thread and updated accordingly
	tps_t foundTPS = NULL;
	queue_iterate(memoryQUEUE, find_tid, (void*)&tid, (void**)&foundTPS);

	// check to see if it exists
	if(foundTPS != NULL) {
		tps_create(); //make a new TPS for the current thread
		tps_t curTID = NULL;
		queue_iterate(memoryQUEUE, find_tid, (void*)pthread_self(), (void**)&curTID);
		// check to see if TPS we just created was found or not
		if(curTID != NULL) {
			// links the new TPS with our target TPS
			tpsLink(curTID, foundTPS->page);
		}
		else if (curTID == NULL){
			return -1;
		}
	} //if TPS we want to clone does not exist
	else if (foundTPS == NULL){
		return -1;
	}
	return 0;
}

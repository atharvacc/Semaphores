#include <stddef.h>
#include <stdlib.h>

#include "queue.h"
#include "sem.h"
#include "thread.h"

struct semaphore {
	int count;
	queue_t BLOCKED;
};

sem_t sem_create(size_t count)
{
	sem_t newSemaphore = malloc(sizeof(struct semaphore));
	newSemaphore->count =count;
	newSemaphore->BLOCKED = queue_create();
	return newSemaphore;
}

int sem_destroy(sem_t sem)
{
	if(sem==NULL || queue_length(sem->BLOCKED)!=0 ) {
		return -1;
	}
	else{
		queue_destroy(sem->BLOCKED);
		free(sem);
	}
	return 0;
}

int sem_down(sem_t sem)
{
	if(sem == NULL){ 
		return -1;
	}// If sem is NULL
	else if(sem->count > 0){ 
		sem->count--;
	}// If can decrement then decrement
	else if (sem->count == 0){
		pthread_t* curTid = malloc(sizeof(pthread_t));
		curTid = pthread_self();
		queue_enqueue(sem->BLOCKED, curTid);
		thread_block();
	} // Else if count is 0 then block and wait
	return 0;
}

int sem_up(sem_t sem)
{
	if(sem == NULL){
		return -1;
	}
}

int sem_getvalue(sem_t sem, int *sval)
{
	/* TODO Phase 1 */
}


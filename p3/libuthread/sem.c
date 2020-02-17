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
	enter_critical_section();

	if( sem==NULL || queue_length(sem->BLOCKED)!=0 ) {
		exit_critical_section();
		return -1;
	}
	else{
		queue_destroy(sem->BLOCKED);
		free(sem);
	}
	exit_critical_section();
	return 0;
}

int sem_down(sem_t sem)
{
	enter_critical_section();
	if(sem == NULL){ 
		exit_critical_section();
		return -1;
	}// If sem is NULL
	else if(sem->count != 0){ 
		sem->count--;
	}// If can decrement then decrement
	else if (sem->count <= 0){
		pthread_t* curTid = malloc(sizeof(pthread_t));
		curTid = pthread_self();
		queue_enqueue(sem->BLOCKED, curTid);
		thread_block();	
	} // Else if count is 0 or negative then block and wait
	exit_critical_section();
	return 0;
}

int sem_up(sem_t sem)
{
	enter_critical_section();
	pthread_t* blockedTid = malloc(sizeof(pthread_t));
	if(sem == NULL){ 
		free(blockedTid);
		exit_critical_section();
		return -1;
	}// If sem is null then return -1
	
	else if (queue_dequeue(sem->BLOCKED,blockedTid) == 0){
		thread_unblock(blockedTid);
		free(blockedTid);
	}// Then dequee was succesfull 
	else{
		free(blockedTid);
		sem->count++;
	}
	exit_critical_section();
	return 0;
}

int sem_getvalue(sem_t sem, int *sval)
{
	enter_critical_section();
	if(sem == NULL || sval == NULL){
		exit_critical_section();
		return -1;
	}
	else if(sem->count > 0){
		*sval = sem->count;
		
	}
	else if (sem->count == 0){
		*sval = -1 * queue_length(sem->BLOCKED);
	}
	exit_critical_section();
	return 0;
}


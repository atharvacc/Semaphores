# Project 3 Report

## Semaphores API
We implemented a Semaphore API as described by the header file to ensure that there is good synchroonizations between the shared variables. It is
upto the user to use the API appropriately to ensure that the synchronization happens as expected.

### Implementation details
We use the following data structure for semaphores.
```
struct semaphore {
	int count;
	queue_t BLOCKED;
};
```
The count value is incremented, and decremented using up and down. This is used to block and unblock the threads as appropriate. The blocked queue contains the all of the blocked threads in DOWN, when we try to access a critical section, but the semaphore currently doesn't let it access it. The threads in the blocked queue are released in UP.

#### sem_t sem_create(size_t count)
This initializes the new semaphore, assigns the count to the semaphore to assign an upperbound on the number of threads that can access the area protected by a semaphore, and also initializes the BLOCKED queue for the semaphore.

#### int sem_destroy(sem_t sem)
If the sem provided is NULL, or if a thread is being blocked in the BLOCKED queue for the semaphore, we return -1. or we destroy the queue and then free the semaphore.
We consider this to be a critical section, since it is possible that two threads are trying to destroy the same semaphore. However, we could have a race condition in this which could try to free sem twice when it has already been freed.

#### int sem_down(sem_t sem)
We use this when trying to access something protected by a semaphore. If the semaphore is NULL then we return -1. If not, then if the the count is not 0, then we just decrement it by 1 adnd return 0. Otherwise the thread needs to be blocked, so we block the thread and add it to the BLOCKED queue.
We consider this to be a critical section, since two threads could get the same value of count, and decrement it only once instead of decrementing it twice.(RACE CONDITION)

#### int sem_up(sem_t sem)
We use this once we are done w the resource, and want to allow other threads to access it if they want to. 
- If the sem provided is NULL, we return -1. 
- If there is a thread in the blocked queue waiting to access the thread, then we release it and don't incrememnt the count.
- If nothing is waiting, then we can increment the counter.
This is a critical section as well for the same reason as sem_down. Also, the unblocking in the BLOCKED queue could be affected by a RACE condition as well.

#### int sem_getvalue(sem_t sem, int *sval)
Does what is provided in the header file. Nothing that needs further explanation here. It is consider a critical section as well since the sem value can be modified if a context switch happens after entering it.




### Testing
For testing, we primarily used the three test files given, sem_count.c, sem_buffer.c, and sem_prime.c. We started testing using sem_count.c and then moved onto testing with the other two files. We made sure that we entered and exited the critical sections in the right places in each of the functions.



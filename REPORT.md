# Project 3 Report

## Semaphores API
We implemented a Semaphore API as described by the header file to ensure that th
ere is good synchroonizations between the shared variables. It is
upto the user to use the API appropriately to ensure that the synchronization ha
ppens as expected.

### Implementation details
We use the following data structure for semaphores.
```
struct semaphore {
	int count;
	queue_t BLOCKED;
};
```
The count value is incremented, and decremented using up and down. This is used 
to block and unblock the threads as appropriate. The blocked queue contains the 
all of the blocked threads in DOWN, when we try to access a critical section, bu
t the semaphore currently doesn't let it access it. The threads in the blocked q
ueue are released in UP.

#### sem_create(size_t count)
This initializes the new semaphore, assigns the count to the semaphore to assign
 an upperbound on the number of threads that can access the area protected by a 
semaphore, and also initializes the BLOCKED queue for the semaphore.

#### sem_destroy(sem_t sem)
If the sem provided is NULL, or if a thread is being blocked in the BLOCKED queu
e for the semaphore, we return -1. or we destroy the queue and then free the sem
aphore.
We consider this to be a critical section, since it is possible that two threads
 are trying to destroy the same semaphore. However, we could have a race conditi
on in this which could try to free sem twice when it has already been freed.

#### sem_down(sem_t sem)
We use this when trying to access something protected by a semaphore. If the sem
aphore is NULL then we return -1. If not, then if the the count is not 0, then w
e just decrement it by 1 adnd return 0. Otherwise the thread needs to be blocked
, so we block the thread and add it to the BLOCKED queue.
We consider this to be a critical section, since two threads could get the same 
value of count, and decrement it only once instead of decrementing it twice.(RAC
E CONDITION)

#### sem_up(sem_t sem)
We use this once we are done w the resource, and want to allow other threads to 
access it if they want to. 
- If the sem provided is NULL, we return -1. 
- If there is a thread in the blocked queue waiting to access the thread, then w
e release it and don't incrememnt the count.
- If nothing is waiting, then we can increment the counter.
This is a critical section as well for the same reason as sem_down. Also, the un
blocking in the BLOCKED queue could be affected by a RACE condition as well.

#### sem_getvalue(sem_t sem, int *sval)
Does what is provided in the header file. Nothing that needs further explanation
 here. It is consider a critical section as well since the sem value can be modi
fied if a context switch happens after entering it.
### Testing
For testing, we primarily used the three test files given, sem_count.c, sem_buff
er.c, and sem_prime.c. We made sure the output matched what was expected.
##TPS
The goal of the TPS API is to provide a single private and protected memory page
 (i.e. 4096 bytes) to each thread that requires it.
### Implemention details
Initialize we used a single struct 
```
struct memoryStorage{
	pthread_t* tid;
	char* mmapPtr;
};
```
This sufficed for the first two parts. However, in the last part we had to do Co
py-on-Write, therefore, we added a different struct called page, and moved the p
ointer to the memory there. Thus, in the final version we have two structs defin
ed as follows:
```
struct page {
	char* mmapPtr;
	int refCounter;
};


struct memoryStorage{
	pthread_t* tid;
	struct page* myPage;
};
```
We also have a memoryQueue that checks the mapping from TID to the memory assign
ed to it, if assigned.
#### tps_init(int segv)
- We set the signal handler for SIGBUS, and SIGSEGV. We use the code provided by
 the professor for this section with a minor modification. We added a function c
alled "find_char" that finds the matching starting address to p_fault. If found 
we print out the tps error message and switch back to the original handler.
```
static int find_char(void *data, void *arg)
{
    struct memoryStorage  *a = (struct memoryStorage*)data;
    char* match = (char*)arg;
    if (a->myPage->mmapPtr == match){
        return 1;
    }
    return 0;
}
```
- We also initialize the memoryQueue here.
#### tps_create(void)
We create a new memoryStorage struct, assign a new memory using mmap, and set th
e refCounter to 1 here. This struct is added to the memoryQUEUE to know which th
read ids have been assigned memory.
#### tps_destroy(void)
If the tid is present in the memory queue, it destroys the associated struct wit
h the TID and the reference to the page. It also decrements the reference counte
r for the page. If tid is not present then we return -1.
#### tps_read(size_t offset, size_t length, void *buffer)
If the length + offset is greater than the size of the memory then we return -1,
 or if buffer is null. Otherwise, we use mprotect to allow reading from the memo
ry, and read the data to the buffer. We use a function find_tid to find the tid 
within the queue.
```
static int find_tid(void *data, void *arg)
{
    struct memoryStorage  *a = (struct memoryStorage*)data;
    pthread_t match = (pthread_t)arg;
    if (a->tid == match){
        return 1;
    }
    return 0;
}
```
#### tps_write(size_t offset, size_t length, void *buffer)
There are 4 major cases in this. 
- If the the offset + length is greater than tps size or buffer is null return -
1.
- If on finding the tid in tempStorage, we can't find the TID then we return -1.
- If we find the TID then we need to check the counter value.
If the refCounter value is 1. Then no copy on write was done, thus, we can modif
y the value directly.
- If the refCounter value was superior to 1, then we need to copy the contents a
nd then write to it. To do this, we first create decrement the refCounter. Then 
we allocate memory to it. After which, we copy the contents from the old page to
 this memory, and then write the data from the buffer to the memory. After this,
 we set the permission to PROT_NONE.
#### tps_clone(pthread_t tid)
- For the first part, we checked if the currently running thread had memoryStora
ge in the queue. If it did, then we return -1.
- If the tid provided is not present in the queue then we return -1 as well.
- For the first part, we created a new memory block and copies the contents to i
t, and subsequently enqued this memoryStorage to the queue.
```
void *mptr = NULL;
	mptr = mmap(NULL,TPS_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE| MAP_ANON
,-1,0); // Create memory
	if(mptr == MAP_FAILED){
		return -1;
	} // If failed in memory creation
	struct memoryStorage *cloneStorage = malloc(sizeof(struct memoryStorage)
);
	cloneStorage->myPage = malloc(sizeof(struct page));
	cloneStorage->tid = curTid;
	cloneStorage->myPage->mmapPtr = (char*) mptr;
	struct memoryStorage *temp = (struct memoryStorage*) tempStorage1;
	mprotect(temp->myPage->mmapPtr, TPS_SIZE, PROT_READ);
	memcpy( cloneStorage->myPage->mmapPtr, temp->myPage->mmapPtr, TPS_SIZE);
	mprotect(cloneStorage->myPage->mmapPtr, TPS_SIZE, PROT_NONE);
	mprotect(temp->myPage->mmapPtr, TPS_SIZE, PROT_NONE);
	queue_enqueue(memoryQUEUE, cloneStorage);
```
 - However, for the subsequent part, we had to create a page struct and enable C
opy-on-Write, so we just created a new struct and assigned the page to point to 
the page already made for the tid. We only create a new copy in write now.

# Project 3 Report

## Phase 1:
In this phase, we had to implement a semaphore with grab and release functionalities.

### Data Structure
We have a struct that stores information about the count of the semaphore and also a queue of the blocked threads.
```
struct semaphore {
	int count;
	queue_t BLOCKED;
};
```
### Workflow


### Testing
For testing, we primarily used the three test files given, sem_count.c, sem_buffer.c, and sem_prime.c. We started testing using sem_count.c and then moved onto testing with the other two files. We made sure that we entered and exited the critical sections in the right places in each of the functions.

## Phase 2:
Our goal for this phase was to provide a single private and protected memory page to each thread that requires it.

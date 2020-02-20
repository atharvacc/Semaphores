#include <assert.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sem.h>
#include <tps.h>

static pthread_t previous_tid;
static char msg[TPS_SIZE] = "Hello world!\n";
static char msg1[TPS_SIZE] = "world!\n";
static char msg2[TPS_SIZE] = "Howdy world!\n";

void *latest_mmap_addr; // global variable to make the address returned by mmap
void *__real_mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off);
void *__wrap_mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off)
{
    latest_mmap_addr = __real_mmap(addr, len, prot, flags, fildes, off);
    return latest_mmap_addr;
}

/* Function to test the tps_create function */
void* test_create(void* ptr) {

    // create a TPS
    assert(!tps_create());
    printf("TPS successfully created\n");

    // try to make another tps and verify failure
    assert(tps_create() == -1);
    printf("TPS clone successfully failed\n");

    // destroy the tps
    tps_destroy();
    printf("TPS successfully destroyed\n");
    return 0;
}


/* Function to test the tps_read and tps_write function */
void* test_read_and_write(void* ptr) {
    char *buffer = malloc(TPS_SIZE);

    // thread doesn't have a tps yet
    assert(tps_read(0, TPS_SIZE, buffer) == -1);
    assert(tps_write(0, TPS_SIZE, buffer) == -1);

    // create a TPS
    tps_create();
    assert(latest_mmap_addr != NULL);

    // writes a few bytes
    assert(!tps_write(0, TPS_SIZE, msg));
    printf("Message written to TPS successfully\n");
    memset(buffer, 0, TPS_SIZE);

    // Read from TPS into a buffer
    assert(!tps_read(0, TPS_SIZE, buffer));
    printf("Message read from TPS successfully\n");

    // compare with the original message
    assert(!memcmp(msg, buffer, TPS_SIZE));
    printf("Our message was written to the buffer\n");

    // writing operation is out of bounds
  	assert(tps_write(0, TPS_SIZE+1, buffer) == -1);
  	assert(tps_write(1, TPS_SIZE, buffer) == -1);

    // reading operation is out of bounds
  	assert(tps_read(0, TPS_SIZE+1, buffer) == -1);

  	// buffer is null
  	assert(tps_write(0, TPS_SIZE, NULL) == -1);
    assert(tps_read(0, TPS_SIZE, NULL) == -1);

    // destroy the tps
    tps_destroy();
    return 0;
}

/* Function to test the tps_read and tps_write with an offset */
void* test_offset(void* ptr) {
	  char *buffer = malloc(TPS_SIZE);
    memset(buffer, 0, TPS_SIZE);

	  // create a TPS
    tps_create();
    assert(latest_mmap_addr != NULL);

    memset(buffer, 0, TPS_SIZE);
  	tps_write(6, strlen(msg1) + 1, msg1);
  	tps_read(0, TPS_SIZE, buffer);
  	assert(!memcmp(msg2, buffer, TPS_SIZE));

    // destroy the TPS
    assert(tps_destroy() == 0);
    return 0;
}

/* Function to test the tps_clone for non-existent TPS */
void* test_clone_bad_tid(void* ptr) {

    // Create a TPS
    assert(!tps_create());
    printf("TPS successfully created\n");

    // Try to clone a non-existent TID
    assert(tps_clone(0) == -1);
    printf("TPS clone successfully failed\n");

    // destroy the tps
    tps_destroy();
    printf("TPS successfully destroyed\n");

    return 0;
}

/* Function to test the tps_read after tps_clone */
void* test_clone_read(void* ptr) {

    char *buffer = malloc(TPS_SIZE);

    // Clone a TPS
    assert(tps_clone(previous_tid) == 0);
    assert(latest_mmap_addr != NULL);
    printf("Current thread successfully cloned target TPS\n");

    // Read from TPS into a buffer
    assert(!tps_read(0, TPS_SIZE, buffer));
    printf("Message read from TPS successfully\n");

    // compare with the original message
    assert(!memcmp(msg, buffer, TPS_SIZE));
    printf("The message written to buffer is same as orignal message\n");

    // destroy the tps
    tps_destroy();
    printf("tps successfully destroyed\n");

    return 0;
}

/* Function to test the basic tps_clone */
void* test_clone(void* ptr) {

    pthread_t tid;

    // Create a TPS
    assert(!tps_create());
    printf("TPS successfully created\n");

    // writes a few bytes into it
    assert(!tps_write(0, TPS_SIZE, msg));
    printf("Message written to TPS successfully\n");

    previous_tid = pthread_self();
    pthread_create(&tid, NULL, test_clone_read, NULL);
    pthread_join(tid, NULL);

    // destroy the TPS
    tps_destroy();
    printf("TPS successfully destroyed\n");

    return 0;
}

/* Function to test the segfault handler */
void *test_seg(void * ptr)
{
  char *tps_addr;
	/* Create TPS */
	tps_create();

	/* Get TPS page address as allocated via mmap() */
	tps_addr = latest_mmap_addr;

	/* Cause an intentional TPS protection error */
	tps_addr[0] = '\0';

	printf("intentional segfault test passed!\n");
	
	return 0;
}

int main(int argc, char*argv[]) {
    pthread_t tid;

    /* Init TPS API */
    tps_init(1);
    /* Create thread 1: test_create and wait */
    printf("\nTesting TPS create and destroy\n");
    pthread_create(&tid, NULL, test_create, NULL);
    pthread_join(tid, NULL);

    /* Create thread 2: test_read_and_write and wait */
    printf("\nTesting TPS read and write\n");
    pthread_create(&tid, NULL, test_read_and_write, NULL);
    pthread_join(tid, NULL);

    
    /* Create thread 3: test_offset and wait 
    printf("\nTesting TPS read and write at specified offset\n");
    pthread_create(&tid, NULL, test_offset, NULL);
    pthread_join(tid, NULL); */

    /* Create thread 4: test_clone */
    printf("\nTesting TPS clone\n");
    pthread_create(&tid, NULL, test_clone, NULL);
    pthread_join(tid, NULL);

    /* Create thread 5: test_clone_bad_tid */
    printf("\nTesting TPS cloning an non-existent TID\n");
    pthread_create(&tid, NULL, test_clone_bad_tid, NULL);
    pthread_join(tid, NULL);

    /* Test the TPS protection */
    printf("\nTesting TPS protection handler\n\n");
    test_seg(NULL);
    return 0;
}

#include <assert.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tps.h>
#include <sem.h>

#define INIT 0
#define CREATE 1
#define READWRITE 2
#define READWRITEOFFSET 3
#define CLONEREAD 4
#define CLONE 5
#define CLONEBADTID 6
#define MYT 7

static pthread_t previous_tid;
static char msg1[TPS_SIZE] = "Hello world!\n";
static char msg2[TPS_SIZE] = "Goodbye world!\n";
static char msg3[TPS_SIZE] = "Hello earth!\n";
static char msg4[TPS_SIZE] = "Goodbye earth\n";

void *latest_mmap_addr; // global variable to make the address returned by mmap

void *__real_mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off);
void *__wrap_mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off)
{
    latest_mmap_addr = __real_mmap(addr, len, prot, flags, fildes, off);
    return latest_mmap_addr;
}

void test_init() {
	// Verify we can initiate without malloc failure
	assert(!tps_init(0));
	// Verify initiating while already initiated causes an error
	assert(tps_init(0) == -1);
	printf("init tests passed!\n");
}

/* Function to test the tps_create function
 * Function does not return anything and there are no arguments
 */
void* test_create(void* ptr) {
  assert(!tps_create());
	// Try to make another tps and verify failure (can't make 2 for one thread)
	assert(tps_create() == -1);
	tps_destroy();
	return NULL;
}

/* Function to test the tps_read and tps_write function
 * Function does not return anything and there are no arguments
 */
void* test_read_write(void* ptr) {
    // buffer to read into
    char *buffer = malloc(TPS_SIZE);

    // create a TPS
    tps_create();

    // writes a few bytes into it
    assert(tps_write(0, TPS_SIZE, msg1) == 0);
    printf("Message written to TPS successfully\n");
    memset(buffer, 0, TPS_SIZE);

    // Read from TPS into a buffer
    assert(tps_read(0, TPS_SIZE, buffer) == 0);
    printf("Message read from TPS successfully\n");

    // compare with the original message
    assert(!memcmp(msg1, buffer, TPS_SIZE));
    printf("The message written to buffer is same as orignal message\n");

    // destroy the tps
    tps_destroy();
    printf("TPS successfully destroyed\n");
    return NULL;
}

/* Function to test the tps_read and tps_write at different offsets
 * Function does not return anything and there are no arguments
 */
void* test_read_write_with_offset(void* ptr) {
	char *buffer = malloc(TPS_SIZE);
    memset(buffer, 0, TPS_SIZE);

	// create a TPS
    assert(tps_create() == 0);
    assert(latest_mmap_addr != NULL);

    // write msg2
    assert(tps_write(5, 7, msg2) == 0);
    printf("Message2 written to TPS offset 5 successfully\n");

    // write msg3
    assert(tps_write(12, 7, msg3) == 0);
    printf("Message3 written to TPS offset 12 successfully\n");

    // read from TPS into a buffer
    assert(tps_read(5, 7, buffer) == 0);
    printf("Message1 read from TPS offset 5 successfully\n");

    // Compare with original message
    assert(!memcmp(msg2, buffer, TPS_SIZE));
    printf("The message written to buffer at offset 5 is same as orignal message\n");
    memset(buffer, 0, TPS_SIZE);

    // Read from TPS into a buffer
    assert(tps_read(12, 7, buffer) == 0);
    printf("Message3 read from TPS offset 12 successfully\n");

    // Compare with the original message
    assert(!memcmp(msg3, buffer, TPS_SIZE));
    printf("The message written to buffer at offset 12 is same as orignal message\n");

    // Write msg4
    assert(tps_write(19, 4, msg4) == 0);
    printf("Message4 written to TPS offset 19 successfully\n");
    memset(buffer, 0, TPS_SIZE);

    // Read from TPS into buffer
    assert(tps_read(19, 4, buffer) == 0);
    printf("Message4 read from TPS offset 19 successfully\n");

    // compare with the original message
    assert(!memcmp(msg4, buffer, TPS_SIZE));
    printf("The message written to buffer at offset 19 is same as orignal message\n");

    // destroy the TPS
    assert(tps_destroy() == 0);
    printf("TPS successfully destroyed\n");
    return 0;
}

/* Function to test the tps_clone for non-existent TPS
 * Function does not return anything and there are no arguments
 */
void* test_clone_bad_tid(void* ptr) {

    // Create a TPS
    assert(tps_create() == 0);
    assert(latest_mmap_addr != NULL);
    printf("TPS clone successfully created\n");

    // Try to clone a non-existent TID
    assert(tps_clone(0) == -1);
    printf("TPS clone successfully failed\n");

    // destroy the tps
    assert(tps_destroy() == 0);
    printf("tps successfully destroyed\n");

    return 0;
}

/* Function to test the tps_read after tps_clone
 * Function does not return anything and there are no arguments
 */
void* test_clone_read(void* ptr) {

    char buffer[TPS_SIZE];

    // Clone a TPS
    assert(tps_clone(previous_tid) == 0);
    assert(latest_mmap_addr != NULL);
    printf("Current thread successfully cloned target TPS\n");

    // Read from TPS into a buffer
    assert(tps_read(0, TPS_SIZE, buffer) == 0);
    printf("Message read from TPS successfully\n");

    // compare with the original message
    assert(!memcmp(msg1, buffer, TPS_SIZE));
    printf("The message written to buffer is same as orignal message\n");

    // destroy the tps
    assert(tps_destroy() == 0);
    printf("tps successfully destroyed\n");

    return 0;
}

/* Function to test the basic tps_clone
 * Function does not return anything and there are no arguments
 */
void* test_clone(void* ptr) {

    pthread_t tid;

    // Create a TPS
    assert(tps_create() == 0);
    assert(latest_mmap_addr != NULL);
    printf("TPS successfully created\n");

    // writes a few bytes into it
    assert(tps_write(0, TPS_SIZE, msg1) == 0);
    printf("Message written to TPS successfully\n");

    previous_tid = pthread_self();
    pthread_create(&tid, NULL, test_clone_read, NULL);
    pthread_join(tid, NULL);

    // destroy the TPS
    assert(tps_destroy() == 0);
    printf("TPS successfully destroyed\n");

    return 0;
}

/* Function to test the segfault handler */
void *my_thread(void *ptr)
{
  char *tps_addr;
	/* Create TPS */
	tps_create();
	/* Get TPS page address as allocated via mmap() */
	tps_addr = latest_mmap_addr;
	/* Cause an intentional TPS protection error  */
	tps_addr[0] = '\0';
 	printf("segfault test passed!\n");
}

static unsigned int get_argv(char *argv)
{
  long int returnval = strtol(argv, NULL, 0);
  if (returnval == LONG_MIN) {
    perror("strtol");
    exit(1);
  } else if (returnval == LONG_MAX) {
    perror("strtol");
    exit(1);
  }
  return returnval;
}

int main(int argc, char **argv) {
    int testNum = -1;
    if (argc > 1)
      testNum = get_argv(argv[1]);

    switch(testNum) {
      case INIT:
        test_init();
        break;
      case CREATE:
        tps_init(1);
        test_create();
        break;
      case READWRITE:
        tps_init(1);
        test_read_write();
        break;
      case READWRITEOFFSET:
        tps_init(1);
        test_read_write_with_offset();
        break;
      case CLONEREAD:
        tps_init(1);
        test_clone_read();
        break;
      case CLONE:
        tps_init(1);
        test_clone();
        break;
      case CLONEBADTID:
        tps_init(1);
        test_clone_bad_tid();
        break;
      case MYT:
        tps_init(1);
        my_thread();
        break;
      default:
        test_init(1);
        test_create();
        test_read_write();
        test_read_write_with_offset();
        test_clone_read();
        test_clone();
        test_clone_bad_tid();
        my_thread();
        printf("\nAll tests passed!\n");
    }
    return 0;
}

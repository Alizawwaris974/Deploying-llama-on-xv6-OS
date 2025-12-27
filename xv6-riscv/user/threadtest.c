
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int counter = 0;
int lock;

void increment(void *arg) {
  int i;
  mutex_lock(&lock);
  printf("Thread running, arg=%d\n", (int)(uint64)arg);
  mutex_unlock(&lock);

  for (i = 0; i < 1000; i++) {
    mutex_lock(&lock);
    counter++;
    mutex_unlock(&lock);
  }

  mutex_lock(&lock);
  printf("Thread exiting\n");
  mutex_unlock(&lock);
  thread_exit();
}

int main(int argc, char *argv[]) {
  int t1, t2;
  
  mutex_init(&lock);
  
  printf("Starting threadtest\n");
  
  t1 = thread_create(increment, (void*)1);
  if(t1 < 0) {
    printf("thread_create failed\n");
    exit(1);
  }
  printf("Created thread 1, tid=%d\n", t1);

  t2 = thread_create(increment, (void*)2);
  if(t2 < 0) {
    printf("thread_create failed\n");
    exit(1);
  }
  printf("Created thread 2, tid=%d\n", t2);
  
  thread_join(t1);
  printf("Joined thread 1\n");
  
  thread_join(t2);
  printf("Joined thread 2\n");
  
  printf("Counter: %d (expected 2000)\n", counter);
  
  if(counter == 2000) {
    printf("TEST PASSED\n");
    exit(0);
  } else {
    printf("TEST FAILED\n");
    exit(1);
  }
}

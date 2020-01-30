#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include "ktiming.h"
 
#include <cilk/cilk_api.h>
#include <sys/sysinfo.h>

#ifndef RAND_MAX
#define RAND_MAX 32767
#endif

#define TIMING_COUNT 5

static unsigned long rand_nxt = 0;

static inline unsigned long my_rand(void) {

  rand_nxt = rand_nxt * 1103515245 + 12345;
  return rand_nxt;
}

static inline void my_srand(unsigned long seed) {

  rand_nxt = seed;
}

#define swap(a, b) \
{ \
  long tmp;\
  tmp = a;\
  a = b;\
  b = tmp;\
}

static void scramble_array(long *arr, unsigned long size) {

  unsigned long i;
  unsigned long j;

  for (i = 0; i < size; ++i) {
    j = my_rand();
    j = j % size;
    swap(arr[i], arr[j]);
  }
}

static void fill_array(long *arr, unsigned long size, long start) {

  unsigned long i;

  my_srand(1);
  /* first, fill with integers 1..size */
  for (i = 0; i < size; ++i) {
    arr[i] = i + start;
  }

  /* then, scramble randomly */
  scramble_array(arr, size);
}

static void 
check_result(long *res, unsigned long size, long start, char *name) {
  printf("Now check result ... \n");
  int success = 1;
  for (unsigned long i = 0; i < size; i++) {
    if (res[i] != (start+i))
      success = 0;
  }
  if(!success)
    fprintf(stdout, "%s sorting FAILURE!\n", name);
  else 
    fprintf(stdout, "%s sorting successful.\n", name);
}

/* forward declaration */
long * cilk_sort(long *array, unsigned long size, unsigned int num_threads);
long * pthread_sort(long *array, unsigned long size, unsigned int num_threads);

void call_cilk_sort(long *array, unsigned long size, long start,
    unsigned int num_threads, int check) {

  clockmark_t begin, end;
  uint64_t elapsed_time[TIMING_COUNT];
  long *cilk_res = NULL;

  for(int i=0; i < TIMING_COUNT; i++) {
    /* calling the sort implemented using cilk */
    begin = ktiming_getmark();
    cilk_res = cilk_sort(array, size, num_threads);
    end = ktiming_getmark();
    elapsed_time[i] = ktiming_diff_usec(&begin, &end);

    __sync_synchronize();
    if(check && i==0) { 
      check_result(cilk_res, size, start, "cilk_sort"); 
    }
    // free the array if not the same
    if(array != cilk_res) { free(cilk_res); }
    fill_array(array, size, start);
  }

  print_runtime(elapsed_time, TIMING_COUNT);
}

void call_pthread_sort(long *array, unsigned long size, long start,
    unsigned int num_threads, int check) {

  clockmark_t begin, end;
  uint64_t elapsed_time[TIMING_COUNT];
  long *pthread_res = NULL;

  for(int i=0; i < TIMING_COUNT; i++) {
    /* calling the sort implemented using cilk */
    begin = ktiming_getmark();
    pthread_res = pthread_sort(array, size, num_threads);
    end = ktiming_getmark();
    elapsed_time[i] = ktiming_diff_usec(&begin, &end);

    __sync_synchronize();
    if(check && i==0) { 
      check_result(pthread_res, size, start, "pthread_sort"); 
    }
    // free the array if not the same
    if(array != pthread_res) { free(pthread_res); }
    fill_array(array, size, start);
  }

  print_runtime(elapsed_time, TIMING_COUNT);
}

void print_usage(int argc, char **argv) {
  if(argc >= 1 && argv[0][0] != '\0') {
    fprintf(stderr, "Usage: %s", argv[0]);
  } else {
    fprintf(stderr, "Usage: ./sort");
  }

  fprintf(stderr,
          " <n> [<P>] [<cilk>] [<pthread>]\n"
          "\tn - size of the array to be sorted (integer, required)\n"
          "\tP - number of threads to use for sorting (integer, optional)\n"
          "\tcilk - 1 to enable cilk sorting, 0 to disable (boolean, optional)\n"
          "\tpthread - 1 to enable pthread sorting, 0 to disable (boolean, optional)\n\n" 
  );
}

int main(int argc, char **argv) {

  /* default benchmark options */
  int check = 1;
  int run_cilk = 1;
  int run_pthread = 0;
  unsigned long size = 0;
  uintptr_t num_threads = get_nprocs();

  long *array;

  if(argc < 2) {
    print_usage(argc, argv);
    exit(EXIT_FAILURE);
  }
  
  int err = sscanf(argv[1], "%lu", &size);
  if (err == 0 || err == EOF) {
    fprintf(stderr, "ERROR: Unable to parse argument <n> (got %s)\n", argv[1]);
    print_usage(argc, argv);
    exit(EXIT_FAILURE);
  }

  if (size < 1) {
    fprintf(stderr, "ERROR: Invalid array size (%lu)\n", size);
    exit(EXIT_FAILURE);
  }

  if (argc > 2) {
    err = sscanf(argv[2], "%" SCNuPTR "", &num_threads);
    if (err == 0 || err == EOF) {
      // The parameter is optional, so just raise a warning
      fprintf(stderr,
              "WARNING: Error parsing number of threads, [<P>]; defaulting to"
              "%" PRIuPTR " (got %s)\n", num_threads, argv[2]
      );
    }
  }
  printf("Number of threads: %" PRIuPTR "\n", num_threads);

  if (argc > 3) {
    err = sscanf(argv[3], "%d", &run_cilk);
    if (err == 0 || err == EOF) {
      fprintf(stderr, "ERROR: Unable to parse argument [<cilk>] (got %s)\n", argv[3]);
      exit(EXIT_FAILURE);
    }
  }

  if (argc > 4) {
    err = sscanf(argv[4], "%d", &run_pthread);
    if (err == 0 || err == EOF) {
      fprintf(stderr, "ERROR: Unable to parse argument [<cilk>] (got %s)\n", argv[4]);
      exit(EXIT_FAILURE);
    }
  }

  long start = my_rand();

  fprintf(stdout, "Creating a randomly permuted array of size %ld.\n", size);
  array = (long *) malloc(size * sizeof(long));
  fill_array(array, size, start);

  if (run_cilk) {
    // Set the number of threads cilk should use
    char num_threads_str[64];
    sprintf(num_threads_str, "%" PRIuPTR "", num_threads);
    __cilkrts_set_param("nworkers", num_threads_str);
    __cilkrts_init();
    call_cilk_sort(array, size, start, num_threads, check);
    __cilkrts_end_cilk();
  }

  if (run_pthread) {
    call_pthread_sort(array, size, start, num_threads, check);
  }

  free(array);

  return 0;
}


/**Update this file with the starter code**/
#include <stdlib.h>
#include <sys/time.h> /* for gettimeofday system call */
#include <limits.h>
#include "lab.h"

int zero = 0;

/**
 * @brief Standard insertion sort that is faster than merge sort for small array's
 *
 * @param A The array to sort
 * @param p The starting index
 * @param r The ending index
 */
static void insertion_sort(int A[], int p, int r)
{
  int j;

  for (j = p + 1; j <= r; j++)
    {
      int key = A[j];
      int i = j - 1;
      while ((i > p - 1) && (A[i] > key))
        {
	  A[i + 1] = A[i];
	  i--;
        }
      A[i + 1] = key;
    }
}


void mergesort_s(int A[], int p, int r)
{
  if (r - p + 1 <=  INSERTION_SORT_THRESHOLD)
    {
      insertion_sort(A, p, r);
    }
  else
    {
      int q = (p + r) / 2;
      mergesort_s(A, p, q);
      mergesort_s(A, q + 1, r);
      merge_s(A, p, q, r);
    }

}

void merge_s(int A[], int p, int q, int r)
{
  int *B = (int *)malloc(sizeof(int) * (r - p + 1));

  int i = p;
  int j = q + 1;
  int k = 0;
  int l;

  /* as long as both lists have unexamined elements */
  /*  this loop keeps executing. */
  while ((i <= q) && (j <= r))
    {
      if (A[i] < A[j])
        {
	  B[k] = A[i];
	  i++;
        }
      else
        {
	  B[k] = A[j];
	  j++;
        }
      k++;
    }

  /* now only at most one list has unprocessed elements. */
  if (i <= q)
    {
      /* copy remaining elements from the first list */
      for (l = i; l <= q; l++)
        {
	  B[k] = A[l];
	  k++;
        }
    }
  else
    {
      /* copy remaining elements from the second list */
      for (l = j; l <= r; l++)
        {
	  B[k] = A[l];
	  k++;
        }
    }

  /* copy merged output from array B back to array A */
  k = 0;
  for (l = p; l <= r; l++)
    {
      A[l] = B[k];
      k++;
    }

  free(B);
}

/**
 * @brief helper function for true_mergesort_mt
 */
void *mergesortMTRoutine(void * params){
    mergesort_mt(((int**)params)[0], ((int**)params)[1][0], ((int**)params)[2][0]);
    pthread_exit(NULL);
}

void true_mergesort_mt(int *A, int n, int num_thread){
  if(1 >= num_thread || n <= INSERTION_SORT_THRESHOLD){
    mergesort_s(A, 0, n-1);
    return;
  }

  int childNumThread = num_thread - (num_thread / 2);
  int childArrayLength = n-(n/2);

  int * childThreadArgs[3] = {A+(n/2), &childArrayLength, &childNumThread};

  pthread_t childThread;

  pthread_create(&childThread, NULL, mergesortMTRoutine, childThreadArgs);

  mergesort_mt(A, n/2, num_thread-childNumThread);

  pthread_join(childThread, NULL);

  merge_s(A, 0, (n/2) - 1, n-1);
}

void mergesort_mt(int *A, int n, int num_thread){
    if(n <= num_thread || (num_thread < 2)){//if there are more (or same number) threads than elements, multi-threaded is equivalent to single-threaded
        mergesort_s(A, 0, n-1);
        return;
    }
    int elementsPerThread = n/num_thread;
    int elementsPerThread1 = elementsPerThread + n%num_thread;//additional elements go to the first thread
    int endingIndex = elementsPerThread - 1;
    int endingIndexThread1 = elementsPerThread1 - 1;

    struct parallel_args mergesortArgs[num_thread];

    int zero = 0;
    int* currentArray = A;

    mergesortArgs[0].A = currentArray;
    mergesortArgs[0].start = zero;
    mergesortArgs[0].end = endingIndexThread1;
    pthread_create(&mergesortArgs[0].tid, NULL, parallel_mergesort, mergesortArgs);
    currentArray += (elementsPerThread1);
    for(int i = 1; i < num_thread; i++){
        mergesortArgs[i].A = currentArray;
        mergesortArgs[i].start = zero;
        mergesortArgs[i].end = endingIndex;
        pthread_create(&mergesortArgs[i].tid, NULL, parallel_mergesort, mergesortArgs+i);
        currentArray += (elementsPerThread);
    }

    //join all the threads created above
    for(int i = 0; i < num_thread; i++){
        pthread_join(mergesortArgs[i].tid, NULL);
    }

    int intermediaryArray[n];

    for(int i = 0; i < n; i++){
        int smallestFound = 0;
        for(int i = 0; i < num_thread; i++){
            if(*mergesortArgs[i].A < *mergesortArgs[smallestFound].A){
              smallestFound = i;
            }
        }
        intermediaryArray[i] = *mergesortArgs[smallestFound].A;
        *mergesortArgs[smallestFound].A = INT_MAX;
        mergesortArgs[smallestFound].A += 1;;
    }

    for(int i = 0; i < n; i++){
        A[i] = intermediaryArray[i];
    }    
}

void *parallel_mergesort(void *args){
    mergesort_s(((struct parallel_args *)args)->A, ((struct parallel_args *)args)->start,((struct parallel_args *)args)->end);
    pthread_exit(NULL);
}

double getMilliSeconds()
{
  struct timeval now;
  gettimeofday(&now, (struct timezone *)0);
  return (double)now.tv_sec * 1000.0 + now.tv_usec / 1000.0;
}
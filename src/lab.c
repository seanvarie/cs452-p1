/**Update this file with the starter code**/
#include <stdlib.h>
#include <sys/time.h> /* for gettimeofday system call */
#include "lab.h"

#include <stdio.h>//TODO: remove

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

//TODO: add comment
void *mergesortRoutine(void * params){//TODO: consider edge cases
    mergesort_s(((int**)params)[0], ((int**)params)[1][0], ((int**)params)[2][0]);
    pthread_exit(NULL);
}

//TODO: add comment
void *mergesortMTRoutine(void * params){//TODO: consider edge cases
    mergesort_mt(((int**)params)[0], ((int**)params)[1][0], ((int**)params)[2][0]);
    pthread_exit(NULL);
}

void mergesort_mt(int *A, int n, int num_thread){//TODO: consider edge cases
  if(1 >= num_thread || n <= INSERTION_SORT_THRESHOLD){
    mergesort_s(A, 0, n-1);
    return;
  }

  int childNumThread = num_thread - (num_thread / 2);
  int childArrayLength = n-(n/2);

  int * childThreadArgs[3] = {A+(n/2), &childArrayLength, &childNumThread};

  pthread_t childThread;

  pthread_create(&childThread, NULL, mergesortMTRoutine, childThreadArgs);

  mergesort_mt(A, n/2, num_thread-childNumThread);//TODO: check

  pthread_join(childThread, NULL);

  merge_s(A, 0, (n/2) - 1, n-1);
}

void mergesort_mt_attempt1(int *A, int n, int num_thread){//TODO: consider edge cases, or remove entire function?
    if(n <= num_thread || (num_thread < 2)){//if there are more (or same number) threads than elements, multi-threaded is equivalent to single-threaded
        mergesort_s(A, 0, n-1);
        return;
    }
    int elementsPerThread = n/num_thread;
    int elementsPerThread1 = elementsPerThread + n%num_thread;//additional elements go to the first thread
    int endingIndex = elementsPerThread - 1;
    int endingIndexThread1 = elementsPerThread1 - 1;
    pthread_t threads[num_thread];
    int* mergesortArgs[3*num_thread];
    int zero = 0;
    int* currentArray = A;

    //TODO: remove below
    /*
    printf("\nn: %d\n", n);
    printf("\nnum_thread: %d\n", num_thread);
    printf("\nelementsPerThread: %d\n", elementsPerThread);
    printf("\nelementsPerThread1: %d\n", elementsPerThread1);
    for(int i = 0; i < 3*num_thread; i++){
        int* test = mergesortArgs[i];
    }*/
    //TODO: remvoe above

    //magic number 3 is the number of parameters in mergesort_s
    mergesortArgs[0] = currentArray;
    mergesortArgs[1] = &zero;
    mergesortArgs[2] = &endingIndexThread1;
//    mergesortRoutine(mergesortArgs);//TODO: replace with pthread create TODO: remove
    pthread_create(&threads[0], NULL, mergesortRoutine, mergesortArgs);
    currentArray += (elementsPerThread1);
    for(int i = 1; i < num_thread; i++){
        mergesortArgs[3*i] = currentArray;
        mergesortArgs[(3*i)+1] = &zero;
        mergesortArgs[(3*i)+2] = &endingIndex;
//        mergesortRoutine(mergesortArgs+(3*i));//TODO: replace with pthread create TODO: remove
        pthread_create(&threads[i], NULL, mergesortRoutine, mergesortArgs+(3*i));
        currentArray += (elementsPerThread);
    }

    /*TODO: remove or uncomment
    //join all the pthreads created above
    for(int i = 0; i < num_thread; i++){
        pthread_join(threads[i], NULL);
    } */

    //TODO: refactor to merge arrays more efficiently (i.e., as is done in mergesort)

    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);
    int middle = endingIndexThread1;
    int ending = endingIndexThread1 + elementsPerThread;
//    printf("\n\nmiddle: %d\n", middle);//TODO: remove
//    printf("ending: %d\n", ending);//TODO: remove
    for(int i = 0; i < num_thread-1; i++){
        pthread_join(threads[i], NULL);
        pthread_join(threads[i+1], NULL);
        merge_s(A, zero, middle, ending);
        middle += elementsPerThread;
        ending += elementsPerThread;
//        printf("middle: %d\n", middle);//TODO: remove
//        printf("ending: %d\n", ending);//TODO: remove
    }


//    fprintf(stderr, "ERROR: mergesort_mt not implemented\n");//TODO: implement and remove error message
    
}

double getMilliSeconds()
{
  struct timeval now;
  gettimeofday(&now, (struct timezone *)0);
  return (double)now.tv_sec * 1000.0 + now.tv_usec / 1000.0;
}
#include "lab.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

struct queue {
    int capacity;
    void ** buffer;
    int addIndex;//index (within 'buffer') of next space for an element to be added
    int rmIndex;//index (within 'buffer') of next element to be removed
    bool isEmpty;
    bool shutdownFlag;
    pthread_mutex_t mutex;//mutext so that only 1 queue modifying operation can happen at a time
    sem_t addSemaphore;//semaphore to be waited on in enqueue (only needed to save resources), signaled after an element is added or removed
    sem_t rmSemaphore;//semaphore to be waited on in dequeue (only needed to save resources), signaled after an element is added or removed
};

/**
 * @brief non-atomically (read note) returns true if the queue is full
 * 
 * @note this function is not atomic and may give the wrong result if the 
 * queue's mutex is not owned by the calling thread.
 * 
 * @param q the queue
 * 
 * @return a best guess (read note) as to whether or not the queue is full
 */
bool is_full(queue_t q);

queue_t queue_init(int capacity) {
    queue_t retVal = malloc(sizeof(struct queue));

    retVal->capacity = capacity;
    retVal->buffer = malloc(sizeof(void *) * capacity);
    retVal->addIndex = retVal->rmIndex = 0;
    retVal->isEmpty = true;
    retVal->shutdownFlag = false;
    pthread_mutex_init(&retVal->mutex, NULL);
    sem_init(&retVal->addSemaphore, 0, 1);
    sem_init(&retVal->rmSemaphore, 0, 0);

    return retVal; // Replace with actual queue object
}

void queue_destroy(queue_t q) {
    queue_shutdown(q);
    int semValue = 1;

    while(semValue > 0 && sem_getvalue(&q->addSemaphore, &semValue)){//ensure no threads are waiting on addSemaphor
        sem_post(&q->addSemaphore);
    }
    semValue = 1;

    while(semValue > 0 && sem_getvalue(&q->rmSemaphore, &semValue)){//ensure no threads are waiting on rmSemaphor
        sem_post(&q->rmSemaphore);
        sleep(0);
    }

    sem_destroy(&q->addSemaphore);
    sem_destroy(&q->rmSemaphore);

    pthread_mutex_lock(&q->mutex);
    pthread_mutex_unlock(&q->mutex);
    pthread_mutex_destroy(&q->mutex);

    free(q->buffer);
    free(q);
}

void enqueue(queue_t q, void *data) {
    if(q->shutdownFlag){
        return;
    }

    bool ownLock = false;//true if this thread currently owns q->addMutex
    while(!ownLock){
        if(!is_full(q)){
            ownLock = (0 == pthread_mutex_trylock(&q->mutex));
            if(ownLock && is_full(q)){
                pthread_mutex_unlock(&q->mutex);
                ownLock = false;
                sem_post(&q->rmSemaphore);
            }
            if(!ownLock){
                sem_wait(&q->addSemaphore);
                if(q->shutdownFlag){
                    return;//there is no way that this thread owns the queue's mutex at this point
                }
            }
        }
        else{
            sem_wait(&q->addSemaphore);
            if(q->shutdownFlag){
                return;//there is no way that this thread owns the queue's mutex at this point
            }
        }
    }

    if(q->shutdownFlag){
        pthread_mutex_unlock(&q->mutex);
        ownLock = false;
        return;
    }

    q->buffer[q->addIndex] = data;

    q->addIndex = (q->addIndex + 1)%q->capacity;

    q->isEmpty = false;

    pthread_mutex_unlock(&q->mutex);
    ownLock = false;//technically unnecessary but makes me feel safe

    sem_post(&q->rmSemaphore);
    sem_post(&q->addSemaphore);
}

void *dequeue(queue_t q) {
    void * retVal;

    bool ownLock = false;//true if this thread currently owns q->addMutex
    while(!ownLock){
        if(!is_empty(q)){
            ownLock = (0 == pthread_mutex_trylock(&q->mutex));
            if(ownLock && is_empty(q)){
                pthread_mutex_unlock(&q->mutex);
                ownLock = false;
                sem_post(&q->addSemaphore);
            }
            if(!ownLock){
                sem_wait(&q->rmSemaphore);
            }
        }
        else{
            if(q->shutdownFlag && is_empty(q)){
                sem_post(&q->rmSemaphore);
                return NULL;//there is no way that this thread owns the queue's mutex at this point
            }
            sem_wait(&q->rmSemaphore);
        }
    }

    retVal = q->buffer[q->rmIndex];

    q->rmIndex = (q->rmIndex + 1)%q->capacity;

    q->isEmpty = (q->addIndex == q->rmIndex);

    pthread_mutex_unlock(&q->mutex);
    ownLock = false;//technically unnecessary but makes me feel safe

    sem_post(&q->addSemaphore);
    sem_post(&q->rmSemaphore);

    return retVal;
}

void queue_shutdown(queue_t q) {
    q->shutdownFlag = true;
    sem_post(&q->rmSemaphore);
    sem_post(&q->addSemaphore);
}

bool is_empty(queue_t q) {
    return q->isEmpty;
}

bool is_full(queue_t q) {
    return (0 == q->capacity) || (!q->isEmpty && (q->addIndex == q->rmIndex));
}

bool is_shutdown(queue_t q) {
    return q->shutdownFlag;
}

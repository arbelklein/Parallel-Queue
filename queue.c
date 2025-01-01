#include <stdlib.h>
#include <stdatomic.h>
#include <threads.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "queue.h"

// Node for the queue
typedef struct Node
{
    void *data;
    struct Node *next;
} Node;

// Node for waiting threads queue
typedef struct WaitingThread
{
    cnd_t cond;
    struct WaitingThread *next;
} WaitingThread;

// Queue
typedef struct Queue
{
    Node *head;
    Node *tail;

    atomic_size_t size;
    atomic_size_t waitingSize;
    atomic_size_t visitedSize;

    mtx_t lock;
    cnd_t notEmpty;

    // Queue for handling waiting threads
    WaitingThread *waitingThreadHead;
    WaitingThread *waitingThreadTail;

} Queue;

static Queue queue;

// Remove the first item in the queue (the head of the queue)
void *removeFirst()
{
    Node *temp = queue.head;
    queue.head = queue.head->next;

    // The queue has only one item
    if (queue.head == NULL)
    {
        queue.tail = NULL;
    }

    void *data = temp->data;

    free(temp);

    return data;
}

// Initialize the queue
void initQueue(void)
{
    // Initialize the head and tail
    queue.head = NULL;
    queue.tail = NULL;

    // Initialize the atomic counters to zero
    atomic_init(&queue.size, 0);
    atomic_init(&queue.waitingSize, 0);
    atomic_init(&queue.visitedSize, 0);

    // Initialize the mutex and condition variable
    mtx_init(&queue.lock, mtx_plain);
    cnd_init(&queue.notEmpty);

    // Initialize the waiting threads queue
    queue.waitingThreadHead = NULL;
    queue.waitingThreadTail = NULL;
}

// Destroy the queue
void destroyQueue(void)
{
    // Lock the mutex
    mtx_lock(&queue.lock);

    // Free all nodes in the queue
    while (queue.head != NULL)
    {
        Node *temp = queue.head;
        queue.head = queue.head->next;
        free(temp);
    }
    queue.tail = NULL;

    // Wake Up all waiting threads
    while (queue.waitingThreadHead != NULL)
    {
        WaitingThread *temp = queue.waitingThreadHead;
        queue.waitingThreadHead = queue.waitingThreadHead->next;
        cnd_signal(&temp->cond);
    }
    queue.waitingThreadTail = NULL;

    // Unlock the mutex
    mtx_unlock(&queue.lock);
    mtx_destroy(&queue.lock);
    cnd_destroy(&queue.notEmpty);
}

// Enqueue an item
void enqueue(void *item)
{
    Node *newNode = (Node *)malloc(sizeof(Node));
    newNode->data = item;
    newNode->next = NULL;

    // Lock the mutex
    mtx_lock(&queue.lock);

    // If the queue is empty, update both head and tail
    if (queue.head == NULL)
    {
        queue.head = newNode;
        queue.tail = newNode;
    }

    // If the queue is not empty, insert the item at the end
    else
    {
        queue.tail->next = newNode;
        queue.tail = newNode;
    }

    // Increment the size counter
    atomic_fetch_add(&queue.size, 1);

    // Signal the oldest thread
    if (queue.waitingThreadHead != NULL)
    {
        WaitingThread *oldestThread = queue.waitingThreadHead;
        queue.waitingThreadHead = queue.waitingThreadHead->next;
        if (queue.waitingThreadHead == NULL)
            queue.waitingThreadTail = NULL;

        // Decrement the waiting counter
        atomic_fetch_sub(&queue.waitingSize, 1);
        cnd_signal(&oldestThread->cond);
    }

    // Unlock the mutex
    mtx_unlock(&queue.lock);
}

// Dequeue an item
void *dequeue(void)
{
    // Lock the mutex
    mtx_lock(&queue.lock);

    bool wasWaiting = false;

    // Create a waiting thread for this thread
    WaitingThread waitingThread;
    cnd_init(&waitingThread.cond);
    waitingThread.next = NULL;

    // The queue is empty
    if (queue.head == NULL)
    {
        wasWaiting = true;

        // Add this thread to the waiting threads queue
        if (queue.waitingThreadHead == NULL) // The queue is empty
        {
            queue.waitingThreadHead = &waitingThread;
            queue.waitingThreadTail = &waitingThread;
        }
        else
        {
            queue.waitingThreadTail->next = &waitingThread;
            queue.waitingThreadTail = &waitingThread;
        }

        // Increment the waiting counter
        atomic_fetch_add(&queue.waitingSize, 1);
    }

    // Wait for the queue to be not empty
    while (queue.head == NULL)
    {
        cnd_wait(&waitingThread.cond, &queue.lock);
    }

    void *data = removeFirst();

    // Decrement the size counter
    atomic_fetch_sub(&queue.size, 1);

    // Increment the visited counter
    atomic_fetch_add(&queue.visitedSize, 1);

    // Unlock the mutex
    mtx_unlock(&queue.lock);

    return data;
}

// Try to dequeue an item
bool tryDequeue(void **item)
{
    // Lock the mutex
    mtx_lock(&queue.lock);

    // If the queue is empty return false
    if (queue.head == NULL)
    {
        mtx_unlock(&queue.lock);
        return false;
    }

    void *data = removeFirst();

    // Decrement the size counter
    atomic_fetch_sub(&queue.size, 1);

    // Increment the visited counter
    atomic_fetch_add(&queue.visitedSize, 1);

    // Unlock the mutex
    mtx_unlock(&queue.lock);

    *item = data;

    return true;
}

// Return the current amount of items in the queue
size_t size(void)
{
    return atomic_load(&queue.size);
}

// Return the current amount of threads waiting for the queue to fill
size_t waiting(void)
{
    return atomic_load(&queue.waitingSize);
}

// Return the amount of items that have passed inside the queue
size_t visited(void)
{
    return atomic_load(&queue.visitedSize);
}

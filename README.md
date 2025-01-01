# Parallel-Queue

## Description
This repository contains a library for a generic concurrent FIFO queue that supports enqueue and dequeue operations. The queue is designed to be thread-safe and ensures that operations are performed in a first-in, first-out (FIFO) order. The library provides functions for initializing and destroying the queue, as well as for adding and removing items from the queue. It also includes functions to check the size of the queue, the number of waiting threads, and the total number of items that have passed through the queue.

## Usage
1. Compile the library using `gcc`:
    ```sh
    gcc -O3 -D_POSIX_C_SOURCE=200809 -Wall -std=c11 -pthread -c queue.c
    ```
2. Include the `queue.h` header file in your project and link against the compiled object file.

## Output
The library provides the following functions:
- `void initQueue(void);` - Initializes the queue.
- `void destroyQueue(void);` - Destroys the queue and frees any allocated resources.
- `void enqueue(void* item);` - Adds an item to the queue.
- `void* dequeue(void);` - Removes an item from the queue. Blocks if the queue is empty.
- `bool tryDequeue(void** item);` - Attempts to remove an item from the queue. Returns `true` if successful, `false` if the queue is empty.
- `size_t size(void);` - Returns the current number of items in the queue.
- `size_t waiting(void);` - Returns the current number of threads waiting for the queue to fill.
- `size_t visited(void);` - Returns the total number of items that have passed through the queue.

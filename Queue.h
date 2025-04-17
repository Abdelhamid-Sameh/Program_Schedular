#ifndef QUEUE_H
#define QUEUE_H

#define MAX_SIZE 5

// macro to capture caller location:


// Queue structure
typedef struct {
    int items[MAX_SIZE];
    int front, rear;
} Queue;

// Function declarations
void initQueue(Queue* q);
int isFull(Queue* q);
int isEmpty(Queue* q);
void enqueue(Queue* q, int value);
int dequeue(Queue* q);
int peek(Queue* q);

#endif // QUEUE_H
